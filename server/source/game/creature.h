
#ifndef _GAME_CREATURE_HEADER
#define _GAME_CREATURE_HEADER

// Include
#include <cstdint>
#include <string>
#include <vector>
#include <pugixml.hpp>

// Game
namespace Game {
	// Creature template IDs
	enum class CreatureID : uint64_t {
		BlitzAlpha = 1667741389,
		SageAlpha = 749013658,
		WraithAlpha = 3591937345,
		GoliathAlpha = 3376462303,
		ZrinAlpha = 576185321,
		AraknaAlpha = 2128242117,
		VexAlpha = 3618328019,
		ViperAlpha = 1237326306,
		LuminAlpha = 963807461,
		JinxAlpha = 3914325063,
		Srs42Alpha = 3524487169,
		MagnosAlpha = 2954534111,
		ArborusAlpha = 1770764384,
		TitanAlpha = 3068979135,
		MaldriAlpha = 3911756266,
		RevenantAlpha = 38011465583,
		KrelAlpha = 1392397193,
		AndromedaAlpha = 3877795735,
		MeditronAlpha = 1557965449,
		SavageAlpha = 3660811270,
		BlitzBeta = 454054015,
		SageBeta = 1986637994,
		WraithBeta = 4201941707,
		ZrinBeta = 2530835539,
		GoliathBeta = 3948469269,
		ViperBeta = 3600769362,
		SeraphXSDelta = 4234304479,
		SeraphXSGamma = 2464059380,
		SeraphXSAlpha = 820281267,
		SeraphXSBeta = 2726720353,
		VexBeta = 3918493889,
		LuminBeta = 1475341687,
		AraknaBeta = 818452503,
		SRS42Beta = 2445894411,
		MagnosBeta = 3639041301,
		JinxBeta = 1442915581,
		TorkAlpha = 1957997466,
	};

	// Creature
	struct Creature {
		std::string name;
		std::string nameLocaleId;
		std::string elementType;
		std::string classType;
		std::string pngLargeUrl;
		std::string pngThumbUrl;

		double gearScore = 0;
		double itemPoints = 0;

		uint32_t id = 0;
		uint64_t nounId = 0;
		uint32_t version = 0;

		void Read(const pugi::xml_node& node);
		void Write(pugi::xml_node& node) const;
	};

	// Creatures
	class Creatures {
		public:
			decltype(auto) begin() { return mCreatures.begin(); }
			decltype(auto) begin() const { return mCreatures.begin(); }
			decltype(auto) end() { return mCreatures.end(); }
			decltype(auto) end() const { return mCreatures.end(); }

			auto& data() { return mCreatures; }
			const auto& data() const { return mCreatures; }

			void Read(const pugi::xml_node& node);
			void Write(pugi::xml_node& node) const;

			void Add(uint32_t templateId);

		private:
			std::vector<Creature> mCreatures;
	};
}

#endif
