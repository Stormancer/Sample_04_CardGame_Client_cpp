#pragma once
#include "stormancer.h"

namespace Stormancer
{
	enum class GameConnectionState
	{
		Disconnected = 0,
		Connecting = 1,
		Authenticated = 2,
		Disconnecting = 3,
		Authenticating = 4,

	};
	struct QueryByPseudoResult
	{
		std::string userId;
		std::string pseudo;
		MSGPACK_DEFINE(userId,pseudo)
	};
	class IAuthenticationService
	{
	public:
		virtual std::string authenticationSceneName() = 0;
		virtual void setAuthenticationSceneName(std::string name) = 0;

		virtual  std::string createUserRoute() = 0;
		virtual void setCreateUserRoute(std::string name) = 0;

		virtual  std::string loginRoute() = 0;
		virtual void setLoginRoute(std::string name) = 0;

		virtual pplx::task<void> createAccount(std::string login, std::string password, std::string email, std::string key,std::string pseudo)=0;
		virtual pplx::task<ScenePtr> login(std::string pseudo, std::string password) = 0;
		virtual pplx::task<ScenePtr> login(const std::unordered_map<std::string,std::string> authenticationContext) = 0;
		virtual pplx::task<ScenePtr> impersonate(std::string provider, std::string claimPath, std::string uid, std::string impersonationSecret) = 0;
		virtual pplx::task<ScenePtr> steamLogin(std::string steamTicket) = 0;

		virtual pplx::task<ScenePtr> getAuthenticationScene() = 0;
		
		virtual pplx::task<ScenePtr> getPrivateScene(std::string sceneId) = 0;

		virtual pplx::task<void> requestPasswordChange(std::string email) = 0;
		virtual pplx::task<void> changePassword(std::string email, std::string code, std::string newPassword) = 0;
		
		virtual pplx::task<std::vector<QueryByPseudoResult>> queryByPseudo(std::string pseudo, int size) = 0;

		virtual pplx::task<void> logout() = 0;

		virtual std::string userId() = 0;
		virtual std::string GetUsername() = 0;

		virtual GameConnectionState connectionState() const= 0;
		virtual Action<GameConnectionState>& connectionStateChangedAction() = 0;
		virtual Action<GameConnectionState>::TIterator onConnectionStateChanged(std::function<void(GameConnectionState)> callback) = 0;
	};

	
};
