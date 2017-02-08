#pragma once
#include "stormancer.h"
#include <algorithm>

namespace Stormancer
{

	enum class FriendStatus
	{
		Online,
		Away,
		Disconnected,

	};

	enum class FriendlistStatusConfig
	{
		Away,
		Invisible,
		Online
	};

	enum class FriendUpdateOperation
	{
		Add,
		Remove,
		Update
	};

	class EmptyFriendDetails
	{
	public:
		void load(web::json::value& json);
	};

	template<class T>
	class Friend
	{
	public:
		std::string userId;
		std::string pseudonym;

		std::string customStatusDetails;

		long lastConnected;
		FriendStatus status;

		T details;

		void load(web::json::value &data)
		{
			auto lastConnected = data[L"LastConnected"];
			if (lastConnected != web::json::value::Null)
			{
				this->lastConnected = lastConnected.as_integer();
			}

			auto pseudonym = data[L"Pseudonym"];
			if (pseudonym != web::json::value::Null)
			{
				auto wstr = pseudonym.as_string();
				this->pseudonym = std::string(wstr.begin(),wstr.end());
			}

			auto details = data[L"Details"];
			if (details != web::json::value::Null)
			{
				this->details.load(details);
			}
			auto status = data[L"Status"];

			if (status != web::json::value::Null)
			{
				this->status = static_cast<FriendStatus>(status.as_integer());
			}
		}

		MSGPACK_DEFINE(userId, pseudonym, customStatusDetails, lastConnected, status)
	};
	template<class T>
	struct FriendUpdateCtx
	{
		FriendUpdateOperation operation;
		Friend<T> content;
	};

	struct FriendListUpdateDto
	{
	public:
		std::string list;
		std::string itemId;
		std::string operation;
		web::json::value data;

		MSGPACK_DEFINE(list, itemId, operation, data)
	};

	template<class T>
	class FriendListService
	{
	public:
		FriendListService(Scene* scene)
		{
			_scene = scene;
			_rpc = _scene->dependencyResolver()->resolve<IRpcService>();

			_scene->addRoute("friends.notification", [this](Packetisp_ptr packet) { onNotificationReceived(packet); });
		}
		pplx::task<void> inviteFriend(std::string id)
		{
			return _rpc->rpcVoid("friends.invitefriend"id);
		}

		pplx::task<void> manageInvitation(std::string id, bool accept)
		{
			return _rpc->rpcVoid("friends.acceptfriendinvitation", id, accept);
		}

		pplx::task<void> removeFriend(std::string friendId)
		{
			return _rpc->rpcVoid("friends.removefriend", friendId);
		}
		pplx::task<void> setStatus(FriendlistStatusConfig status, web::json::value details)
		{
			auto json = details.serialize();
			return _rpc->rpcVoid("friends.setstatus", status, json);
		}

		rxcpp::observable<FriendUpdateCtx<T>> onFriendlistUpdate()
		{
			return _onUpdate;
		}

		std::vector<Friend<T>> getFriends()
		{
			return _friends;
		}

		bool tryGetFriend(std::string id, Friend<T>& value)
		{
			for (auto f : _friends)
			{
				if (f.userId == id)
				{
					value = f;
					return true;
				}
			}

			return false;
		}
	private:
		void onNotificationReceived(Stormancer::Packetisp_ptr packet)
		{
			auto observer = _onUpdate.get_subscriber();

			auto dto = packet->readObject<FriendListUpdateDto>();

			auto data = dto.data;
			FriendUpdateCtx<T> ctx;
			if (dto.operation == "add")
			{
				Friend<T> f;
				if (!tryGetFriend(dto.itemId, f))
				{


					f.load(data);
					_friends.push_back(f);
					ctx.content = f;
					ctx.operation = FriendUpdateOperation::Add;
					observer.on_next(ctx);
				}

			}
			else if (dto.operation == "remove")
			{
				std::vector<Friend<T>>::iterator it = std::find_if(_friends.begin(), _friends.end(), [&dto](Friend<T>& f) { return f.userId == dto.itemId; });
				auto f = *it;
				if (it != _friends.end())
				{
					_friends.erase(it);

					ctx.content = f;
					ctx.operation = FriendUpdateOperation::Remove;
					observer.on_next(ctx);
				}
				

			}
			else if (dto.operation == "update"|| dto.operation == "update.status")
			{
				std::vector<Friend<T>>::iterator it = std::find_if(_friends.begin(), _friends.end(), [&dto](Friend<T>& f) { return f.userId == dto.itemId; });
				if (it != _friends.end())
				{
					it->load(data);
				}
				ctx.content = *it;
				ctx.operation = FriendUpdateOperation::Update;
				observer.on_next(ctx);
			}
			

			
		}

		Scene* _scene;
		std::shared_ptr<IRpcService> _rpc;
		std::vector<Friend<T>> _friends;
		rxcpp::subjects::subject<FriendUpdateCtx<T>> _onUpdate;
	};

	template<typename T>
	class FriendListPlugin : public IPlugin
	{
		// Hérité via IPlugin
		virtual STORMANCER_DLL_API void destroy() final
		{
			delete this;
		}

		void sceneCreated(Scene* scene) final
		{
			if (scene)
			{
				auto name = scene->getHostMetadata("stormancer.plugins.friendlist");

				if (name.length() > 0)
				{
					auto service = std::make_shared<FriendListService<T>>(scene);
					scene->dependencyResolver()->registerDependency<FriendListService<T>>(service);
				}
			}
		}

	};
}