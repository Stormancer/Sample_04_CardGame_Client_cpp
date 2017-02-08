#pragma once
#include "headers.h"
#include <stormancer.h>

namespace Stormancer
{
	struct GameServerInformations
	{
	public:
		std::string Ip;
		int Port;

		MSGPACK_DEFINE(Ip, Port);
	};
	struct RoundResult
	{
	public:
		std::string winnerId;

		int winnerScore;
		int loserScore;

		MSGPACK_DEFINE(winnerId, winnerScore, loserScore);
	};

	struct GameResults
	{
	public:
		std::string winnerId;

		bool forfait;
		std::vector<RoundResult> rounds;

		MSGPACK_DEFINE(winnerId, forfait, rounds);
	};

	struct MMRChanges
	{
	public:
		std::string WinnerId;
		int WinnerGain;

		std::string LoserId;
		int LoserGain;

		MSGPACK_DEFINE(WinnerId, WinnerGain, LoserId, LoserGain);
	};

	enum class PlayerStatus
	{
		NotConnected = 0,
		Connected = 1,
		Ready = 2,
		Faulted = 3,
		Disconnected = 4
	};

	struct SessionPlayer
	{
	public:
		SessionPlayer(std::string playerId, PlayerStatus status)
			: PlayerId(playerId), Status(status) {}
		std::string PlayerId;
		PlayerStatus Status;
	};



	struct PlayerUpdate
	{
	public:
		std::string UserId;
		int Status;
		std::string FaultReason;

		MSGPACK_DEFINE(UserId, Status, FaultReason);
	};

	class GameSessionService
	{
	public:
		GameSessionService(Stormancer::Scene* scene);

		pplx::task<std::shared_ptr<Result<GameServerInformations>>> waitServerReady();
		pplx::task<std::shared_ptr<Result<GameServerInformations>>> waitServerReady(pplx::cancellation_token);

		std::vector<SessionPlayer> getConnectedPlayers();

		std::function<void()> onConnectedPlayerChanged(std::function<void(SessionPlayer)> callback);

		template<typename TOut, typename TIn>
		pplx::task<std::shared_ptr<Result<TOut>>> sendGameResults(TIn results)
		{
			auto rpc = _scene->dependencyResolver()->resolve<IRpcService>();
			return rpc->rpc<TIn, TOut>("gamesession.postresults", results);
		}

		pplx::task<void> connect();

		void ready();
	private:
		void unsubscribeConnectedPlayersChanged(Action<SessionPlayer>::TIterator handle);

	private:
		Action<SessionPlayer> _onConnectedPlayersChanged;
		Scene* _scene;
		std::vector<SessionPlayer> _users;
		pplx::task_completion_event<GameServerInformations> _waitServerTce;
	};
}