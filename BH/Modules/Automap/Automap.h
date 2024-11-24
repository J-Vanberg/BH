#pragma once
#include "../../D2Structs.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Drawing.h"
#include "../../AsyncDrawBuffer.h"

enum AutomapReveal {
	AutomapRevealGame = 0,
	AutomapRevealAct,
	AutomapRevealLevel
};

struct LevelList {
	unsigned int levelId;
	unsigned int x, y, act;
};

struct BaseSkill {
	WORD Skill;
	BYTE Level;
};

class Automap : public Module {
	private:
		int lkLinesColor;
		unsigned int reloadConfig;
		bool revealedGame, revealedAct[6], revealedLevel[255];
		std::map<string, unsigned int> monsterColors;
		std::map<string, unsigned int> chestColors;
		std::list<LevelList*> automapLevels;
		Drawing::UITab* settingsTab;
		std::map<DWORD, std::vector<BaseSkill>> Skills;
		Act* lastAct = NULL;

		void Squelch(DWORD Id, BYTE button);

	public:
		Automap();

		void ReadConfig();
		void OnLoad();
		void OnUnload();

		void LoadConfig();

		void OnLoop();
		void OnAutomapDraw();
		void OnGameJoin();
		void OnGamePacketRecv(BYTE* packet, bool *block);

		void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);

		void ResetRevealed();
		void RevealGame();
		void RevealAct(int act);
		void RevealLevel(Level* level);
		void RevealRoom(Room2* room);

		static Level* GetLevel(Act* pAct, int level);
		static AutomapLayer* InitLayer(int level);
};