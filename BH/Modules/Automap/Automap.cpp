#include "../../D2Ptrs.h"
#include "../../D2Helpers.h"
#include "../../D2Stubs.h"
#include "../../D2Intercepts.h"
#include "Automap.h"
#include "../../BH.h"
#include "../../Drawing.h"
#include "../Item/ItemDisplay.h"
#include "../../AsyncDrawBuffer.h"
#include "../Item/Item.h"
#include "../../Modules/GameSettings/GameSettings.h"

#pragma optimize( "", off)

using namespace Drawing;

DrawDirective drawer(true, 5);

bool IsObjectNormalChest(ObjectTxt* obj)
{
	return (obj->nSelectable0 && obj->nAutoMap != 318 && (
		(obj->nOperateFn == 1) ||	//bed, undef grave, casket, sarc
		(obj->nOperateFn == 3) ||	//basket, urn, rockpile, trapped soul
		(obj->nOperateFn == 4) ||	//chest, corpse, wooden chest, buriel chest, skull and rocks, dead barb
		(obj->nOperateFn == 5) ||	//barrel
		(obj->nOperateFn == 7) ||	//exploding barrel
		(obj->nOperateFn == 14) ||	//loose bolder etc....*
		(obj->nOperateFn == 19) ||	//armor stand
		(obj->nOperateFn == 20) ||	//weapon rack
		(obj->nOperateFn == 48) ||	//trapped soul
		(obj->nOperateFn == 51)		//stash
		));
}

bool IsObjectSpecialChest(ObjectTxt* obj)
{
	return (obj->nSelectable0 && (
		(obj->nOperateFn == 33) ||	//wirt
		(obj->nOperateFn == 68) ||	//evil urn
		(obj->nAutoMap == 318)		// shiny chests
		));
}

Automap::Automap() : Module("Automap") {
	ReadConfig();

	monsterColors["Normal"] = 0x5B;
	monsterColors["Minion"] = 0x60;
	monsterColors["Champion"] = 0x91;
	monsterColors["Boss"] = 0x84;

	chestColors["Normal"] = 0x5B;
	chestColors["Special"] = 0x84;

	lkLinesColor = 105;

	ResetRevealed();
}

void Automap::ReadConfig() {
}

void Automap::LoadConfig() {
	ReadConfig();
}

void Automap::OnLoad() {
	ReadConfig();
	/*
	settingsTab = new UITab("Automap", BH::settingsUI);

	new Texthook(settingsTab, 80, 3, "Toggles");
	unsigned int Y = 0;
	int keyhook_x = 150;
	int col2_x = 250;

	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.revealMap.toggle.isEnabled, "Reveal Map");

	vector<string> options;
	options.push_back("Game");
	options.push_back("Act");
	options.push_back("Level");
	new Texthook(settingsTab, 4, (Y += 15), "Reveal Type:");
	new Combohook(settingsTab, keyhook_x, (Y + 2), 2, &App.automap.revealType.uValue, options);

	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showNormalMonsters.toggle.isEnabled, "Show Normal Monsters");
	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showNormalMonsters.toggle.isEnabled, "Show Strong Monsters");
	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showNormalMonsters.toggle.isEnabled, "Show Unique Monsters");

	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showNormalChests.toggle.isEnabled, "Show Normal Chests");
	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showSpecialChests.toggle.isEnabled, "Show Special Chests");
	new Checkhook(settingsTab, 4, (Y += 15), &App.automap.showAutomapOnJoin.toggle.isEnabled, "Show Automap On Join");

	new Texthook(settingsTab, col2_x + 5, 107, "Monster Colors");
	new Colorhook(settingsTab, col2_x, 137, &monsterColors["Normal"], "Normal");
	new Colorhook(settingsTab, col2_x, 152, &monsterColors["Minion"], "Minion");
	new Colorhook(settingsTab, col2_x, 167, &monsterColors["Champion"], "Champion");
	new Colorhook(settingsTab, col2_x, 182, &monsterColors["Boss"], "Boss");
	*/
}

void Automap::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	bool ctrlState = ((GetKeyState(VK_LCONTROL) & 0x80) || (GetKeyState(VK_RCONTROL) & 0x80));
	if (key == 0x52 && ctrlState || key == reloadConfig) {
		*block = true;
		if (up)
			BH::ReloadConfig();
		return;
	}
}

void Automap::OnUnload() {
}

void Automap::OnLoop() {
	// Get the player unit for area information.
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit || !App.automap.revealMap.toggle.isEnabled)
		return;
	
	// Reveal the automap based on configuration.
	switch((AutomapReveal)App.automap.revealType.uValue) {
		case AutomapRevealGame:
			RevealGame();
		break;
		case AutomapRevealAct:
			RevealAct(unit->pAct->dwAct + 1);
		break;
		case AutomapRevealLevel:
			RevealLevel(unit->pPath->pRoom1->pRoom2->pLevel);
		break;
	}
}

void Automap::OnAutomapDraw() {
	UnitAny* player = D2CLIENT_GetPlayerUnit();
	
	if (!player || !player->pAct || player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == 0)
		return;

	if (lastAct != player->pAct){
		lastAct = player->pAct;
		drawer.forceUpdate();
	}
	
	drawer.draw([=](AsyncDrawBuffer &automapBuffer) -> void {
		POINT MyPos;
		Drawing::Hook::ScreenToAutomap(&MyPos,
			D2CLIENT_GetUnitX(D2CLIENT_GetPlayerUnit()),
			D2CLIENT_GetUnitY(D2CLIENT_GetPlayerUnit()));
		for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext) {
			for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext) {
				DWORD xPos, yPos;

				// Draw monster on automap
				if (unit->dwType == UNIT_MONSTER && IsValidMonster(unit)) {
					int color = -1;
					int size = 3;
					
					if (unit->pMonsterData->fChamp && App.automap.showStrongMonsters.toggle.isEnabled) {
						color = monsterColors["Champion"];
					}
					else if (unit->pMonsterData->fMinion && App.automap.showStrongMonsters.toggle.isEnabled) {
						color = monsterColors["Minion"];
					}
					else if (unit->pMonsterData->fBoss && App.automap.showUniqueMonsters.toggle.isEnabled) {
						color = monsterColors["Boss"];
					}
					else if (App.automap.showNormalMonsters.toggle.isEnabled) {
						color = monsterColors["Normal"];
						size = 2;
					}
					else {
						// Not a monster we want to show
						continue;
					}
						
					//Cow king pack
					if (unit->dwTxtFileNo == 391 &&
							unit->pMonsterData->anEnchants[0] == ENCH_MAGIC_RESISTANT &&
							unit->pMonsterData->anEnchants[1] == ENCH_LIGHTNING_ENCHANTED &&
							unit->pMonsterData->anEnchants[3] != 0)
						color = 0xE1;


					xPos = unit->pPath->xPos;
					yPos = unit->pPath->yPos;
					automapBuffer.push([color, size, xPos, yPos]()->void {
						POINT automapLoc;
						Drawing::Hook::ScreenToAutomap(&automapLoc, xPos, yPos);

						// Top left - Bottom right
						Drawing::Linehook::Draw(
							automapLoc.x - size, automapLoc.y - size,
							automapLoc.x + size, automapLoc.y + size,
							color);

						// Top right - Bottom left
						Drawing::Linehook::Draw(
							automapLoc.x + size, automapLoc.y - size,
							automapLoc.x - size, automapLoc.y + size,
							color);
					});
				}

				// Draw chest on automap
				else if (unit->dwType == UNIT_OBJECT && !unit->dwMode /* Not opened */) {
					int color = -1;
					if (IsObjectNormalChest(unit->pObjectData->pTxt) && App.automap.showNormalChests.toggle.isEnabled) {
						color = chestColors["Normal"];
					}
					else if (IsObjectSpecialChest(unit->pObjectData->pTxt) && App.automap.showSpecialChests.toggle.isEnabled) {
						color = chestColors["Special"];
					}
					else {
						// Not a chest we want to show
						continue;
					}
					xPos = unit->pObjectPath->dwPosX;
					yPos = unit->pObjectPath->dwPosY;
					automapBuffer.push([color, xPos, yPos]()->void{
						POINT automapLoc;
						Drawing::Hook::ScreenToAutomap(&automapLoc, xPos, yPos);
						Drawing::Boxhook::Draw(automapLoc.x - 1, automapLoc.y - 1, 2, 2, color, Drawing::BTHighlight);
					});
				}				
			}
		}

		// No idea what this is
		if (lkLinesColor > 0 && player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == MAP_A3_LOWER_KURAST) {
			for(Room2 *pRoom =  player->pPath->pRoom1->pRoom2->pLevel->pRoom2First; pRoom; pRoom = pRoom->pRoom2Next) {
				for (PresetUnit* preset = pRoom->pPreset; preset; preset = preset->pPresetNext) {
					DWORD xPos, yPos;
					int lkLineColor = lkLinesColor;
					if (preset->dwTxtFileNo == 160) {
						xPos = (preset->dwPosX) + (pRoom->dwPosX * 5);
						yPos = (preset->dwPosY) + (pRoom->dwPosY * 5);
						automapBuffer.push([xPos, yPos, MyPos, lkLineColor]()->void{
							POINT automapLoc;
							Drawing::Hook::ScreenToAutomap(&automapLoc, xPos, yPos);
							Drawing::Linehook::Draw(MyPos.x, MyPos.y, automapLoc.x, automapLoc.y, lkLineColor);
						});
					}
				}
			}
		}
	});
}

void Automap::OnGameJoin() {
	ResetRevealed();
	automapLevels.clear();
	*p_D2CLIENT_AutomapOn = App.automap.showAutomapOnJoin.toggle.isEnabled;
}

void Automap::Squelch(DWORD Id, BYTE button) {
	LPBYTE aPacket = new BYTE[7];	//create packet
	*(BYTE*)&aPacket[0] = 0x5d;	
	*(BYTE*)&aPacket[1] = button;	
	*(BYTE*)&aPacket[2] = 1;	
	*(DWORD*)&aPacket[3] = Id;
	D2NET_SendPacket(7, 0, aPacket);

	delete [] aPacket;	//clearing up data

	return;
}

void Automap::OnGamePacketRecv(BYTE *packet, bool *block) {
}

void Automap::ResetRevealed() {
	revealedGame = false;
	for (int act = 0; act < 6; act++)
		revealedAct[act] = false;
	for (int level = 0; level < 255; level++)
		revealedLevel[level] = false;
}

void Automap::RevealGame() {
	// Check if we have already revealed the game.
	if (revealedGame)
		return;

	// Iterate every act and reveal it.
	for (int act = 1; act <= ((*p_D2CLIENT_ExpCharFlag) ? 5 : 4); act++) {
		RevealAct(act);
	}

	revealedGame = true;
}

void Automap::RevealAct(int act) {
	// Make sure we are given a valid act
	if (act < 1 || act > 5)
		return;

	// Check if the act is already revealed
	if (revealedAct[act])
		return;

	UnitAny* player = D2CLIENT_GetPlayerUnit();
	if (!player || !player->pAct)
		return;

	// Initalize the act incase it is isn't the act we are in.
	int actIds[6] = {1, 40, 75, 103, 109, 137};
	Act* pAct = D2COMMON_LoadAct(act - 1, player->pAct->dwMapSeed, *p_D2CLIENT_ExpCharFlag, 0, D2CLIENT_GetDifficulty(), NULL, actIds[act - 1], D2CLIENT_LoadAct_1, D2CLIENT_LoadAct_2);
	if (!pAct || !pAct->pMisc)
		return;

	// Iterate every level for the given act.
	for (int level = actIds[act - 1]; level < actIds[act]; level++) {
		Level* pLevel = GetLevel(pAct, level);
		if (!pLevel)
			continue;
		if (!pLevel->pRoom2First)
			D2COMMON_InitLevel(pLevel);
		RevealLevel(pLevel);
	}

	InitLayer(player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo);
	D2COMMON_UnloadAct(pAct);
	revealedAct[act] = true;
}

void Automap::RevealLevel(Level* level) {
	// Basic sanity checks to ensure valid level
	if (!level || level->dwLevelNo < 0 || level->dwLevelNo > 255)
		return;

	// Check if the level has been previous revealed.
	if (revealedLevel[level->dwLevelNo])
		return;

	InitLayer(level->dwLevelNo);

	// Iterate every room in the level.
	for(Room2* room = level->pRoom2First; room; room = room->pRoom2Next) {
		bool roomData = false;

		//Add Room1 Data if it is not already there.
		if (!room->pRoom1) {
			D2COMMON_AddRoomData(level->pMisc->pAct, level->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1);
			roomData = true;
		}

		//Make sure we have Room1
		if (!room->pRoom1)
			continue;

		//Reveal the room
		D2CLIENT_RevealAutomapRoom(room->pRoom1, TRUE, *p_D2CLIENT_AutomapLayer);

		//Reveal the presets
		RevealRoom(room);

		//Remove Data if Added
		if (roomData)
			D2COMMON_RemoveRoomData(level->pMisc->pAct, level->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1);
	}

	revealedLevel[level->dwLevelNo] = true;
}

void Automap::RevealRoom(Room2* room) {
	//Grabs all the preset units in room.
	for (PresetUnit* preset = room->pPreset; preset; preset = preset->pPresetNext)
	{
		int cellNo = -1;
		
		// Special NPC Check
		if (preset->dwType == UNIT_MONSTER)
		{
			// Izual Check
			if (preset->dwTxtFileNo == 256)
				cellNo = 300;
			// Hephasto Check
			if (preset->dwTxtFileNo == 745)
				cellNo = 745;
		// Special Object Check
		} else if (preset->dwType == UNIT_OBJECT) {
			// Uber Chest in Lower Kurast Check
			if (preset->dwTxtFileNo == 580 && room->pLevel->dwLevelNo == MAP_A3_LOWER_KURAST)
				cellNo = 318;

			// Countess Chest Check
			if (preset->dwTxtFileNo == 371) 
				cellNo = 301;
			// Act 2 Orifice Check
			else if (preset->dwTxtFileNo == 152) 
				cellNo = 300;
			// Frozen Anya Check
			else if (preset->dwTxtFileNo == 460) 
				cellNo = 1468; 
			// Canyon / Arcane Waypoint Check
			if ((preset->dwTxtFileNo == 402) && (room->pLevel->dwLevelNo == 46))
				cellNo = 0;
			// Hell Forge Check
			if (preset->dwTxtFileNo == 376)
				cellNo = 376;

			// If it isn't special, check for a preset.
			if (cellNo == -1 && preset->dwTxtFileNo <= 572) {
				ObjectTxt *obj = D2COMMON_GetObjectTxt(preset->dwTxtFileNo);
				if (obj)
					cellNo = obj->nAutoMap;//Set the cell number then.
			}
		} else if (preset->dwType == UNIT_TILE) {
			LevelList* level = new LevelList;
			for (RoomTile* tile = room->pRoomTiles; tile; tile = tile->pNext) {
				if (*(tile->nNum) == preset->dwTxtFileNo) {
					level->levelId = tile->pRoom2->pLevel->dwLevelNo;
					break;
				}
			}
			level->x = (preset->dwPosX + (room->dwPosX * 5));
			level->y = (preset->dwPosY + (room->dwPosY * 5));
			level->act = room->pLevel->pMisc->pAct->dwAct;
			automapLevels.push_back(level);
		}

		//Draw the cell if wanted.
		if ((cellNo > 0) && (cellNo < 1258))
		{
			AutomapCell* cell = D2CLIENT_NewAutomapCell();

			cell->nCellNo = cellNo;
			int x = (preset->dwPosX + (room->dwPosX * 5));
			int y = (preset->dwPosY + (room->dwPosY * 5));
			cell->xPixel = (((x - y) * 16) / 10) + 1;
			cell->yPixel = (((y + x) * 8) / 10) - 3;

			D2CLIENT_AddAutomapCell(cell, &((*p_D2CLIENT_AutomapLayer)->pObjects));
		}

	}
	return;
}

AutomapLayer* Automap::InitLayer(int level) {
	//Get the layer for the level.
	AutomapLayer2 *layer = D2COMMON_GetLayer(level);

	//Insure we have found the Layer.
	if (!layer)
		return false;

	//Initalize the layer!
	return (AutomapLayer*)D2CLIENT_InitAutomapLayer(layer->nLayerNo);
}

Level* Automap::GetLevel(Act* pAct, int level)
{
	//Insure that the shit we are getting is good.
	if (level < 0 || !pAct)
		return NULL;

	//Loop all the levels in this act
	
	for(Level* pLevel = pAct->pMisc->pLevelFirst; pLevel; pLevel = pLevel->pNextLevel)
	{
		//Check if we have reached a bad level.
		if (!pLevel)
			break;

		//If we have found the level, return it!
		if (pLevel->dwLevelNo == level && pLevel->dwPosX > 0)
			return pLevel;
	}
	//Default old-way of finding level.
	return D2COMMON_GetLevel(pAct->pMisc, level);
}

#pragma optimize( "", on)
