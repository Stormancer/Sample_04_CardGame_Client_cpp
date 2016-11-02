// Sample_04_Cardgame_Client_cpp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "stormancer.h"
#include "Online\AuthenticationPlugin.h"
#include "Online\MatchmakingPlugin.h"
#include "Online\TurnByTurnPlugin.h"

int main()
{
	auto config = Stormancer::Configuration::create("http://api.stormancer.com", "ad912dff-cec9-0fb6-78f8-652d4c093508", "sample-04-cardgame");
	config->addPlugin(new Stormancer::AuthenticationPlugin()); //Adds Authentication
	config->addPlugin(new Stormancer::MatchmakingPlugin()); //Adds matchmaking & gamesession APIs
	config->addPlugin(new Stormancer::TurnByTurnPlugin());

	auto client = Stormancer::Client::createClient(config);

	auto auth = client->dependencyResolver()->resolve<Stormancer::IAuthenticationService>();

	auto scene = auth->steamLogin("test1").get();

	auto matchmakingScene = auth->getPrivateScene("matchmaking-fast").get();

	auto matchmaking = matchmakingScene.lock()->dependencyResolver()->resolve<Stormancer::MatchmakingService>();

	auto tce = pplx::task_completion_event<Stormancer::MatchmakingResponse>{};
	matchmaking->onMatchFound([tce](Stormancer::MatchmakingResponse response)
	{

		tce.set(response);

	});
	matchmaking->findMatch("matchmaking-sample");

	auto mmResponse = pplx::create_task(tce).get();


	int n;
	std::cin >> n;
	client->disconnect().get();
	return 0;
}

