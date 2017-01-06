#include "stdafx.h"
#include "MatchmakingService.h"
//#include <ostringstream>

namespace Stormancer
{
	std::string StringifyBytesArray(std::string bytes, bool hex)
	{
		std::stringstream ss;
		if (hex)
		{
			ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase;
		}
		for (size_t i = 0; i < bytes.length(); ++i)
		{
			if (i)
			{
				ss << ' ';
			}
			ss << std::setw(2) << (int)(byte)bytes[i];
		}
		return ss.str();
	}

	MatchmakingService::MatchmakingService(Stormancer::Scene* scene)
		: _scene(scene),
		_rpcService(scene->dependencyResolver()->resolve<Stormancer::IRpcService>())
	{
		auto sceneLock = _scene;
		sceneLock->addRoute("match.update", [this](Stormancer::Packetisp_ptr packet) {
			byte matchStateByte;
			packet->stream->read((char*)&matchStateByte, 1);
			Stormancer::int32 matchState = matchStateByte;

			_matchState = (MatchState)matchState;

			auto ms = std::to_string(matchState);

			if (_onMatchUpdate)
			{
				_onMatchUpdate(_matchState);
			}

			if (_matchState == MatchState::Success)
			{
				msgpack::unpacked result;

				std::string buffer;
				*packet->stream >> buffer;

				//std::string bufferLisible = StringifyBytesArray(buffer, true);
				//UE_LOG(LogTemp, Error, TEXT("buffer value -----------------> :  %s"), *(FString(bufferLisible.c_str())));

				auto off = msgpack::unpack(result, buffer.data(), buffer.size());
				Internal::MatchmakingResponse mmres;
				result.get().convert(&mmres);

				MatchmakingResponse mmres2;

				mmres2.gameId = mmres.gameId;
				mmres2.optionalParameters = mmres.optionalParameters;
				mmres2.team1 = mmres.team1;
				mmres2.team1 = mmres.team2;


				_onMatchFound(mmres2);
			}
		});

		sceneLock->addRoute("match.parameters.update", [this](Stormancer::Packetisp_ptr packet) {
			std::string buffer;
			*packet->stream >> buffer;

			msgpack::unpacked result;
			auto off = msgpack::unpack(result, buffer.data(), buffer.size());
			std::string provider;
			result.get().convert(&provider);

			//_onMatchParametersUpdate();
		});

		sceneLock->addRoute("match.ready.update", [this](Stormancer::Packetisp_ptr packet) {
			std::string buffer;
			*packet->stream >> buffer;
			msgpack::unpacked result;
			msgpack::unpack(result, buffer.data(), buffer.size());
			Internal::ReadyVerificationRequest readyUpdate;
			result.get().convert(&readyUpdate);

			ReadyVerificationRequest readyUpdate2;
			readyUpdate2.matchId = readyUpdate.matchId;
			readyUpdate2.timeout = readyUpdate.timeout;
			readyUpdate2.membersCountTotal = (int)readyUpdate2.members.size();
			readyUpdate2.membersCountReady = 0;
			for (auto it : readyUpdate2.members)
			{
				readyUpdate2.members[it.first] = it.second;
				if (it.second == Readiness::Ready)
				{
					readyUpdate2.membersCountReady++;
				}
			}

			if (_onMatchReadyUpdate)
			{
				_onMatchReadyUpdate(readyUpdate2);
			}
		});
	}

	MatchmakingService::~MatchmakingService()
	{
		if (_hasSubscription)
		{
			_matchmakingSubscription.unsubscribe();
			_hasSubscription = false;
		}
	}

	void MatchmakingService::onMatchUpdate(std::function<void(MatchState)> callback)
	{
		_onMatchUpdate = callback;
	}

	void MatchmakingService::onMatchReadyUpdate(std::function<void(ReadyVerificationRequest)> callback)
	{
		_onMatchReadyUpdate = callback;
	}

	void MatchmakingService::onMatchFound(std::function<void(MatchmakingResponse)> callback)
	{
		_onMatchFound = callback;
	}

	MatchState MatchmakingService::matchState() const
	{
		return _matchState;
	}

	pplx::task<void> MatchmakingService::findMatch(std::string provider)
	{
		return findMatch([provider](Stormancer::bytestream* stream) {
			msgpack::pack(stream, provider);
		});

	}

	pplx::task<void> MatchmakingService::findMatch(std::string provider, std::string filter)
	{
		return findMatch([provider, filter](Stormancer::bytestream* stream) {
			msgpack::pack(stream, provider);

			auto request = MatchmakingRequest();
			request.filter = filter;
			msgpack::pack(stream, request);
		});
	}

	pplx::task<void> MatchmakingService::findMatch(std::function<void(bytestream*)> writer)
	{
		pplx::task_completion_event<void> tce;

		_isMatching = true;


		rxcpp::observable<Packetisp_ptr> observable = _rpcService->rpc("match.find", writer, PacketPriority::MEDIUM_PRIORITY);

		auto onNext = [this, tce](Stormancer::Packetisp_ptr packet) {
			_isMatching = false;
			if (_hasSubscription)
			{
				_matchmakingSubscription.unsubscribe();
				_hasSubscription = false;
			}
			tce.set();
		};

		auto onError = [this, tce](std::exception_ptr eptr) {
			_isMatching = false;
			if (_hasSubscription)
			{
				_matchmakingSubscription.unsubscribe();
				_hasSubscription = false;
			}
			try
			{
				if (eptr)
				{
					std::rethrow_exception(eptr);
				}
				else
				{
					throw std::runtime_error("Unknown error during mathmaking.");
				}
			}
			catch (std::exception& ex)
			{
				tce.set_exception(ex);
			}
		};

		_matchmakingSubscription = observable.subscribe(onNext, onError);
		_hasSubscription = true;

		return pplx::create_task(tce);
	}

	void MatchmakingService::resolve(bool acceptMatch)
	{
		_scene->sendPacket("match.ready.resolve", [acceptMatch](Stormancer::bytestream* stream) {
			*stream << acceptMatch;
		}, PacketPriority::MEDIUM_PRIORITY, PacketReliability::RELIABLE);
	}

	void MatchmakingService::cancel()
	{
		if (_isMatching)
		{
			_isMatching = false;
			if (_hasSubscription)
			{
				_matchmakingSubscription.unsubscribe();
				_hasSubscription = false;
			}
			else
			{
				_scene->sendPacket("match.cancel", [](Stormancer::bytestream* stream) {}, PacketPriority::MEDIUM_PRIORITY, PacketReliability::RELIABLE);
			}
		}
	}
};
