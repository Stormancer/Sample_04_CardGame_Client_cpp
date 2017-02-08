// Sample_04_Cardgame_Client_cpp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "stormancer.h"
#include <iostream>
#include "Online/AuthenticationPlugin.h"
#include "Online/MatchmakingPlugin.h"
#include "Online/TurnByTurnPlugin.h"
#include "Online/GameSession.h"
#include "Online/FriendListService.h"

int ApplyTransaction(Stormancer::UpdateDto t, int& gameState)
{
	if (t.cmd == "start")
	{
		gameState = t.json_args()[L"seed"].as_integer();
	}
	else if (t.cmd == "add")
	{
		gameState += t.json_args()[L"value"].as_integer();
	}
	return gameState;
}



int main(int argc, char *argv[])
{
	std::string login = "test";
	if (argc >= 2)
	{
		login = std::string(argv[1]);
	}
	//Create network client
	auto config = Stormancer::Configuration::create("http://api.stormancer.com", "ad912dff-cec9-0fb6-78f8-652d4c093508", "sample-04-cardgame");
	config->addPlugin(new Stormancer::AuthenticationPlugin()); //Adds Authentication
	config->addPlugin(new Stormancer::MatchmakingPlugin()); //Adds matchmaking & gamesession APIs
	config->addPlugin(new Stormancer::TurnByTurnPlugin());
	config->addPlugin(new Stormancer::FriendListPlugin<Stormancer::EmptyFriendDetails>());

	auto client = Stormancer::Client::createClient(config);

	//Authenticate using steam (with stub enabled)
	auto auth = client->dependencyResolver()->resolve<Stormancer::IAuthenticationService>();
	std::cout << "Authenticating as '" << login << "'...";
	auto scene = auth->steamLogin(login).get();
	std::cout << "DONE" << std::endl;

	//Connect to the matchmaking scene
	auto matchmakingScene = auth->getPrivateScene("matchmaking-fast").get();
	matchmakingScene.lock()->connect().get();

	auto matchmaking = matchmakingScene.lock()->dependencyResolver()->resolve<Stormancer::MatchmakingService>();

	///Configure a task_completion_event which will fire when a game is found.
	auto tce = pplx::task_completion_event<Stormancer::MatchmakingResponse>{};
	matchmaking->onMatchFound([tce](Stormancer::MatchmakingResponse response)
	{

		tce.set(response);

	});

	if (argc < 3)
	{
		matchmaking->findMatch("matchmaking-sample");
	}
	else
	{
		matchmaking->findMatch("matchmaking-sample", argv[2]);
	}

	std::cout << "waiting for game...";
	Stormancer::MatchmakingResponse mmResponse = pplx::create_task(tce).get();
	std::cout << "FOUND" << std::endl;

	Stormancer::ScenePtr game_scene = auth->getPrivateScene(mmResponse.gameId).get();
	int gameState = 0;
	auto transactionBroker = game_scene.lock()->dependencyResolver()->resolve<Stormancer::TurnByTurnService>();
	bool running = true;
	transactionBroker->onDesyncErrorCallback([&running](std::string error)
	{
		std::cout << "A desynchronization error occured. Details : " << error << std::endl;
		running = false;
	});
	transactionBroker->onUpdateGameCallback([&gameState](Stormancer::UpdateDto update)
	{

		auto newHash = ApplyTransaction(update, gameState);
		std::cout << "game state updated : " << gameState << std::endl;
		return newHash; //Returns the new hash to the server for validation
	});
	transactionBroker->onReplayTLog([&gameState, &running](std::vector<Stormancer::TransactionLogItem> transactions)
	{
		std::cout << "Replay existing transaction log...";
		for (auto t : transactions)
		{
			auto newHash = ApplyTransaction(t.transactionCommand, gameState);
			if (t.hashAvailable && t.resultHash != newHash)
			{
				std::cout << "Desynchronization while playing Transaction log. Expected " << t.resultHash << " obtained " << newHash << std::endl;
				std::string v;
				std::cin >> v;
				running = false;
				break;

			}
		}

	});


	std::cout << "Connecting to game...";
	auto gameSession = game_scene.lock()->dependencyResolver()->resolve<Stormancer::GameSessionService>();

	gameSession->connect().get();//Connect to the game session





	gameSession->ready();//Inform the server we are ready to play
	gameSession->waitServerReady().get();//
	std::cout << "CONNECTED" << std::endl;

	int n;
	while (running)
	{
		std::cout << "Enter number to add to game state." << std::endl;
		std::cin >> n;
		auto json = web::json::value();
		json[L"value"] = n;
		try
		{
			auto t = transactionBroker->submitTransaction(auth->userId(), "add", json);
			t.get();
		}
		catch (std::exception& ex)
		{
			std::cout << ex.what();
		}
	}

	std::cout << "disconnecting...";
	client->disconnect().get();
	std::cout << "DONE" << std::endl;
	return 0;
}


