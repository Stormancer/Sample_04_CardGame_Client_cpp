#pragma once

#include "headers.h"
#include <stormancer.h>

namespace Stormancer
{
	enum class MatchState
	{
		Unknown = -1,
		SearchStart = 0,
		CandidateFound = 1,
		WaitingPlayersReady = 2,
		Success = 3,
		Failed = 4,
		Canceled = 5
	};

	enum class FleetSize
	{
		Unknown = -1,
		Small = 1,
		Medium = 2,
		Large = 3
	};

	enum class Readiness
	{
		Unknown = 0,
		Ready = 1,
		NotReady = 2
	};

	struct Player
	{
	public:

		std::string playerId;
		std::string pseudo;


		MSGPACK_DEFINE(playerId, pseudo);
	};

	struct MatchmakingResponse
	{
	public:
		std::string gameId;
		std::vector<Player> team1;
		std::vector<Player> team2;
		std::vector<std::string> optionalParameters;
	};

	struct ReadyVerificationRequest
	{
	public:
		std::map<std::string, Readiness> members;
		std::string matchId;
		Stormancer::int32 timeout;
		Stormancer::int32 membersCountReady;
		Stormancer::int32 membersCountTotal;
	};

	struct MatchmakingRequest
	{
	public:
		std::string filter;

		MSGPACK_DEFINE(filter);
	};

	// Msgpack doesn't support using C++11 enum class so we use int32 instead in this temporary internal functions
	namespace Internal
	{
		struct MatchmakingResponse
		{
		public:
			std::string gameId;
			std::vector<Player> team1;
			std::vector<Player> team2;
			std::vector<std::string> optionalParameters;


			MSGPACK_DEFINE(gameId, team1, team2, optionalParameters);
		};

		struct ReadyVerificationRequest
		{
		public:
			std::map<std::string, Stormancer::int32> members;
			std::string matchId;
			Stormancer::int32 timeout;

			MSGPACK_DEFINE(members, matchId, timeout);
		};
	}

	class MatchmakingService
	{
	public:
		MatchmakingService(Stormancer::Scene* scene);
		~MatchmakingService();

		MatchmakingService(const MatchmakingService& other) = delete;
		MatchmakingService(const MatchmakingService&& other) = delete;
		MatchmakingService& operator=(const MatchmakingService&& other) = delete;

		void onMatchUpdate(std::function<void(MatchState)> callback);
		void onMatchReadyUpdate(std::function<void(ReadyVerificationRequest)> callback);
		void onMatchFound(std::function<void(MatchmakingResponse)> callback);

		MatchState matchState() const;

		pplx::task<void> findMatch(std::string provider);
		pplx::task<void> findMatch(std::string provider, std::string filter);

		void resolve(bool acceptMatch);

		void cancel();

	private:
		Stormancer::Scene* _scene;
		std::shared_ptr<Stormancer::IRpcService> _rpcService = nullptr;
		bool _isMatching = false;
		bool _hasSubscription = false;
		rxcpp::subscription _matchmakingSubscription;
		std::function<void(MatchState)> _onMatchUpdate;
		std::function<void(ReadyVerificationRequest)> _onMatchReadyUpdate;
		std::function<void(MatchmakingResponse)> _onMatchFound;
		MatchState _matchState = MatchState::Unknown;
		pplx::task<void> findMatch(std::function<void(bytestream*)> writer);

	};
};
