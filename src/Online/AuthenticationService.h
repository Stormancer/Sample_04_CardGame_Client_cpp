#pragma once
#include "IAuthenticationService.h"

namespace Stormancer
{
	struct LoginResult
	{
	public:
		std::string errorMsg;
		bool success;
		std::string token;
		std::string userId;
		std::string username;

	public:
		MSGPACK_DEFINE(errorMsg, success, token, userId, username);
	};
	struct CreateUserParameters
	{


	public:
		/// <summary>
		/// Player login
		/// </summary>
		std::string login;

		/// <summary>
		/// password stored in db (can/should be hash)
		/// </summary>
		std::string password;

		/// <summary>
		/// user email for password recovery 
		/// </summary>
		std::string email;

		/// <summary>
		/// Json user data
		/// </summary>
		std::string userData;

	public:
		MSGPACK_DEFINE(login, password, email, userData);
	};

	struct ChangePasswordParameters
	{
	public:
		std::string email;
		std::string code;
		std::string newPassword;
	public:
		MSGPACK_DEFINE(email, code, newPassword);
	};

	class AuthenticationService : public IAuthenticationService
	{
	public:
		AuthenticationService(Client* client);
		virtual ~AuthenticationService();

		std::string authenticationSceneName();
		void setAuthenticationSceneName(std::string name);

		std::string createUserRoute();
		void setCreateUserRoute(std::string name);

		std::string loginRoute();
		void setLoginRoute(std::string name);

		virtual pplx::task<void> createAccount(std::string login, std::string password, std::string email, std::string key, std::string pseudo) override;
		virtual pplx::task<ScenePtr> login(std::string email, std::string password) override;
		virtual pplx::task<ScenePtr> login(const std::unordered_map<std::string, std::string> authenticationContext) override;
		virtual pplx::task<ScenePtr> getPrivateScene(std::string sceneId) override;
		virtual pplx::task<ScenePtr> steamLogin(std::string steamTicket) override;

		//Impersonate an user using the impersonation plugin. The plugin should be disabled in production environments.
		virtual pplx::task<ScenePtr> impersonate(std::string provider, std::string claimPath, std::string uid, std::string impersonationSecret) override;

		virtual pplx::task<std::vector<QueryByPseudoResult>> queryByPseudo(std::string pseudo, int size) override;
		

		virtual pplx::task<void> requestPasswordChange(std::string email) override;
		virtual pplx::task<void> changePassword(std::string email, std::string code, std::string newPassword) override;

		virtual pplx::task<ScenePtr> getAuthenticationScene() override;

		virtual pplx::task<void> logout() override;

		std::string userId();
		std::string GetUsername();

		GameConnectionState connectionState() const;
		Action<GameConnectionState>& connectionStateChangedAction();
		Action<GameConnectionState>::TIterator onConnectionStateChanged(std::function<void(GameConnectionState)> callback);

	private:
		void setConnectionState(GameConnectionState state);
	private:
		std::string _authenticationSceneName = "authenticator";
		std::string _createUserRoute = "provider.loginpassword.createAccount";
		std::string _loginRoute = "login";
		bool _authenticated = false;
		bool _authenticationInProgress = false;
		bool _authenticationSceneRetrieving = false;
		std::string _userId;
		std::string _username;

		Client* _client;
		pplx::task<ScenePtr> _authenticationScene;



		Action<GameConnectionState> _onConnectionStateChanged;
		GameConnectionState _connectionState;
	};
};
