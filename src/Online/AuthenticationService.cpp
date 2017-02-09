#include "stdafx.h"
#include "AuthenticationService.h"


namespace Stormancer
{



	AuthenticationService::AuthenticationService(Client* client)
		: _client(client)
	{
		client->GetConnectionStateChangedObservable().subscribe([this](ConnectionState state)
		{
			switch (state)
			{
			case ConnectionState::Connecting:
			case ConnectionState::Disconnecting:
			case ConnectionState::Disconnected:
				this->setConnectionState((GameConnectionState)state);
				break;
			default:
				break;
			}
		});
	}

	AuthenticationService::~AuthenticationService()
	{}

	std::string AuthenticationService::authenticationSceneName()
	{
		return _authenticationSceneName;
	}

	void AuthenticationService::setAuthenticationSceneName(std::string name)
	{
		_authenticationSceneName = name;
	}

	std::string AuthenticationService::createUserRoute()
	{
		return _createUserRoute;
	}

	void AuthenticationService::setCreateUserRoute(std::string name)
	{
		_createUserRoute = name;
	}

	std::string AuthenticationService::loginRoute()
	{
		return _loginRoute;
	}

	void AuthenticationService::setLoginRoute(std::string name)
	{
		_loginRoute = name;
	}

	pplx::task<ScenePtr> AuthenticationService::login(std::string email, std::string password)
	{
		std::unordered_map<std::string, std::string> authContext{ { "provider", "loginpassword" }, { "login", email },{"password",password } };
		return login(authContext);
	}

	pplx::task<ScenePtr> AuthenticationService::steamLogin(std::string steamTicket)
	{
		std::unordered_map<std::string, std::string> authContext{ { "provider", "steam" }, { "ticket", steamTicket } };
		return login(authContext);
	}
	pplx::task<void> AuthenticationService::createAccount(std::string login, std::string password, std::string email, std::string key, std::string pseudo)
	{
		CreateUserParameters rq;
		rq.email = email;
		rq.login = login;
		rq.password = password;
		rq.userData = "{\"key\":\"" + key + "\",\"pseudo\":\"" + pseudo + "\"}";
		return getAuthenticationScene()
			.then([this,rq](ScenePtr scenePtr)
		{
			auto scene = scenePtr.lock();
			auto rpcService = scene->dependencyResolver()->resolve<IRpcService>();
			return rpcService->rpc<CreateUserParameters, LoginResult>(this->_createUserRoute, rq);
		})
			.then([](LoginResult loginResult)
		{
			if (!loginResult.success)
			{
				ILogger::instance()->log(LogLevel::Error, "AuthenticationService", "An error occured while creating an account.", loginResult.errorMsg);
				throw std::runtime_error(loginResult.errorMsg);
			}
		});
	}

	pplx::task<ScenePtr> AuthenticationService::login(const std::unordered_map<std::string, std::string> authenticationContext)
	{
		if (_authenticated)
		{
			return pplx::task_from_exception<ScenePtr>(std::runtime_error("Authentication service already authenticated."));
		}

		return getAuthenticationScene().then([this, authenticationContext](ScenePtr scenePtr)
		{
			auto scene = scenePtr.lock();
			this->setConnectionState(GameConnectionState::Authenticating);
			auto rpcService = scene->dependencyResolver()->resolve<IRpcService>();

			return rpcService->rpc<const std::unordered_map<std::string, std::string>, LoginResult>(_loginRoute, authenticationContext);
		})
			.then([this](LoginResult loginResult)
		{
			if (loginResult.success)
			{
				this->setConnectionState(GameConnectionState::Authenticated);
				_userId = loginResult.userId;
				_username = loginResult.username;
				return _client->getScene(loginResult.token);
			}
			else
			{
				this->setConnectionState(GameConnectionState::Disconnected);
				throw std::runtime_error(loginResult.errorMsg);
			}
		});
	}

	pplx::task<ScenePtr> AuthenticationService::getAuthenticationScene()
	{
		if (!_authenticationSceneRetrieving)
		{
			_authenticationSceneRetrieving = true;

			_authenticationScene = _client->getPublicScene(_authenticationSceneName)
				.then([](ScenePtr scene)
			{
				return scene.lock()->connect().then([scene]()
				{
					return scene;
				});
			});
		}
		return _authenticationScene;
	}


	pplx::task<void> AuthenticationService::logout()
	{
		if (_authenticated)
		{
			_authenticated = false;
			return getAuthenticationScene().then([](ScenePtr scene)
			{
				return scene.lock()->disconnect();
			});
		}
		else
		{
			return pplx::task_from_result();
		}
	}

	std::string AuthenticationService::userId()
	{
		return _userId;
	}

	std::string AuthenticationService::GetUsername()
	{
		return _username;
	}

	pplx::task<ScenePtr> AuthenticationService::getPrivateScene(std::string sceneId)
	{
		return this->getAuthenticationScene().then([sceneId, this](ScenePtr authScene)
		{
			auto rpcService = authScene.lock()->dependencyResolver()->resolve<IRpcService>();

			return rpcService->rpc<std::string, std::string>("sceneauthorization.gettoken", sceneId);
		})
			.then([this](std::string token)
		{
			return _client->getScene(token);
		});

	}


	//*
	//Connection state
	//* 

	Action<GameConnectionState>& AuthenticationService::connectionStateChangedAction()
	{
		return _onConnectionStateChanged;
	}

	Action<GameConnectionState>::TIterator AuthenticationService::onConnectionStateChanged(std::function<void(GameConnectionState)> callback)
	{
		return _onConnectionStateChanged.push_back(callback);
	}

	GameConnectionState AuthenticationService::connectionState() const
	{
		return _connectionState;
	}
	void AuthenticationService::setConnectionState(GameConnectionState state)
	{
		if (_connectionState != state)
		{
			_connectionState = state;
			_onConnectionStateChanged(state);
		}
	}


	pplx::task<void> AuthenticationService::requestPasswordChange(std::string email)
	{
		return this->getAuthenticationScene().then([email](ScenePtr authScene)
		{
			auto rpcService = authScene.lock()->dependencyResolver()->resolve<IRpcService>();
			return rpcService->rpcVoid<std::string>("provider.loginpassword.requestPasswordRecovery", email);
		});
	}
	pplx::task<void> AuthenticationService::changePassword(std::string email, std::string code, std::string newPassword)
	{
		auto parameter = ChangePasswordParameters();
		parameter.email = email;
		parameter.code = code;
		parameter.newPassword = newPassword;

		return this->getAuthenticationScene().then([parameter](ScenePtr authScene)
		{
			auto rpcService = authScene.lock()->dependencyResolver()->resolve<IRpcService>();
			return rpcService->rpcVoid<ChangePasswordParameters>("provider.loginpassword.resetPassword", parameter);
		});
	}

	pplx::task<ScenePtr> AuthenticationService::impersonate(std::string provider, std::string claimPath, std::string uid, std::string secret)
	{
		std::unordered_map<std::string, std::string> authContext{
			{ "provider", "impersonation" },
			{ "secret", secret } ,
			{"impersonated-provider",provider},
			{"claimPath",claimPath},
			{"claimValue",uid} };
		return login(authContext);
	}
	pplx::task<std::vector<QueryByPseudoResult>> AuthenticationService::queryByPseudo(std::string pseudo, int size)
	{
		return this->_authenticationScene.then([=](Stormancer::ScenePtr scene) {
			auto s = scene.lock()
		
		});
	}
};
