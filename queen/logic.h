/* ScummVM - Scumm Interpreter
 * Copyright (C) 2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#ifndef QUEENLOGIC_H
#define QUEENLOGIC_H

#include "common/util.h"
#include "queen/defs.h"
#include "queen/structs.h"
#include "queen/verb.h"

namespace Queen {

enum RoomDisplayMode {
	RDM_FADE_NOJOE  = 0, // fade in, no Joe
	RDM_FADE_JOE    = 1, // Joe is to be displayed
	RDM_NOFADE_JOE  = 2, // screen does not dissolve into view
	RDM_FADE_JOE_XY = 3  // display Joe at the current X, Y coords
};

struct ZoneSlot {
	bool valid;
	Box box;
};

struct GameConfig {
	int musicVolume;
	bool textToggle;
	int talkSpeed;
};


/*!
	Each object/item in game has a state field. 
	(refer to ObjectData and ItemData).
	
	<table>
		<tr>
			<td>Name</td>
			<td>Bits</td>
			<td>Description</td>
		</tr>	
		<tr>
			<td>USE</td>
			<td>10</td>
			<td>Use</td>
		</tr>
		<tr>
			<td>TALK</td>
			<td>9</td>
			<td>Talk</td>
		</tr>
		<tr>
			<td>ON</td>
			<td>8</td>
			<td>On/Off</td>
		</tr>
		<tr>
			<td>DEF</td>
			<td>7,6,5,4</td>
			<td>Default verb command</td>
		</tr>
		<tr>
			<td>DIR</td>
			<td>3,2</td>
			<td>Direction faced</td>
		</tr>
		<tr>
			<td>GRAB</td>
			<td>1,0</td>
			<td>Grab Direction</td>
		</tr>
	</table>
*/
struct State {

	static Direction findDirection(uint16 state);
	static StateTalk findTalk(uint16 state);
	static StateGrab findGrab(uint16 state);
	static StateOn   findOn(uint16 state);
	static Verb      findDefaultVerb(uint16 state);
	static StateUse  findUse(uint16 state);

	static void alterOn(uint16 *objState, StateOn state);
	static void alterDefaultVerb(uint16 *objState, Verb v);
};


class Command;
class Debug;
class Display;
class Input;
class Graphics;
class Resource;
class Sound;
class Walk;

class Logic {

public:
	Logic(Resource *resource, Graphics *graphics, Display *display, Input *input, Sound *sound);
	~Logic();

	uint16 currentRoom() const { return _currentRoom; }
	void currentRoom(uint16 room) { 
		if (room >= 1 && room <= _numRooms)
			_currentRoom = room; 
		else
			error("Invalid room number: %i", room);
	}

	uint16 oldRoom() const { return _oldRoom; }
	void oldRoom(uint16 room) { 
		if (room <= _numRooms)
			_oldRoom = room; 
		else
			error("Invalid room number: %i", room);
	}

	uint16 newRoom() const { return _newRoom; }
	void newRoom(uint16 room) { 
		if (room <= _numRooms)
			_newRoom = room; 
		else
			error("Invalid room number: %i", room);
	}

	ObjectData *objectData(int index) const;
	uint16 roomData(int room) const;
	uint16 objMax(int room) const;
	GraphicData *graphicData(int index) const;
	ItemData *itemData(int index) const { return &_itemData[index]; }
	uint16 itemDataCount() const { return _numItems; }

	uint16 findBob(uint16 obj);
	uint16 findFrame(uint16 obj);
	uint16 objectForPerson(uint16 bobnum) const; // OBJ_PERSON
	WalkOffData *walkOffPointForObject(uint16 obj) const;

	Area *area(int room, int num) const;
	Area *currentRoomArea(int num) const;
	uint16 areaMax(int room) const;
	uint16 currentRoomAreaMax() const;
	uint16 walkOffCount() const;
	WalkOffData *walkOffData(int index) const;
	uint16 currentRoomObjMax() const { return _objMax[_currentRoom]; }
	uint16 currentRoomData() const { return _roomData[_currentRoom]; }
	ObjectDescription *objectDescription(uint16 objNum) const { return &_objectDescription[objNum]; }
	uint16 objectDescriptionCount() const { return _numObjDesc; }

	uint16 joeFacing() const { return _joe.facing; }
	uint16 joeX() const { return _joe.x; }
	uint16 joeY() const { return _joe.y; }
	JoeWalkMode joeWalk() const { return _joe.walk; }
	uint16 joeScale() const { return _joe.scale; }
	uint16 joeCutFacing() const { return _joe.cutFacing; }
	uint16 joePrevFacing() const { return _joe.prevFacing; }

	void joeFacing(uint16 dir);
	void joeX(uint16 x);
	void joeY(uint16 y);
	void joeWalk(JoeWalkMode walking);
	void joeScale(uint16 scale);
	void joeCutFacing(uint16 dir);
	void joePrevFacing(uint16 dir);
	
	const char *joeResponse(int i) const { return _joeResponse[i]; }

	int16 gameState(int index);
	void gameState(int index, int16 newValue);

	TalkSelected *talkSelected(int index) { return _talkSelected + index; }

	const char *roomName(uint16 roomNum) const { 
		if (roomNum >= 1 && roomNum <= _numRooms)
			return _roomName[roomNum];
		else
			error("Invalid room number: %i", roomNum);
	}

	const char *objectName(uint16 objNum) const { return _objName[objNum]; }
	const char *objectTextualDescription(uint16 objNum) const { return _objDescription[objNum]; }

	uint16 numFrames() { return _numFrames; }

	int talkSpeed() const { return _settings.talkSpeed; }

	void zoneSet(uint16 screen, uint16 zone, uint16 x1, uint16 y1, uint16 x2, uint16 y2);
	void zoneSet(uint16 screen, uint16 zone, const Box& box);
	uint16 zoneIn(uint16 screen, uint16 x, uint16 y) const;
	uint16 zoneInArea(uint16 screen, uint16 x, uint16 y) const;
	void zoneClearAll(uint16 screen);
	void zoneSetup();
	void zoneSetupPanel();
	Box &zoneBox(uint16 screen, uint16 index) { return _zones[screen][index].box; } 

	void roomErase();
	void roomSetupFurniture(); // SETUP_FURNITURE()
	void roomSetupObjects(); // DISP_OBJECTS
	uint16 roomRefreshObject(uint16 obj); // REDISP_OBJECT
	void roomSetup(const char *room, int comPanel, bool inCutaway);
	void roomDisplay(uint16 room, RoomDisplayMode mode, uint16 joeScale, int comPanel, bool inCutaway); // DISP_ROOM

	uint16 findScale(uint16 x, uint16 y);

	int16 entryObj() const { return _entryObj; }
	void entryObj(int16 obj) { _entryObj = obj; }

	void personSetData(int16 noun, const char *actorName, bool loadBank, Person *pp); // SET_PERSON_DATA
	uint16 personSetup(uint16 noun, uint16 curImage); // SETUP_PERSON
	uint16 personAllocate(uint16 noun, uint16 curImage); // ALLOCATE_PERSON

	uint16 animCreate(uint16 curImage, const Person *person); // CREATE_ANIM
	void animErase(uint16 bobNum);
	void animReset(uint16 bobNum);
	void animSetup(const GraphicData *gd, uint16 firstImage, uint16 bobNum, bool visible); // FIND_GRAPHIC_ANIMS
	uint16 personFrames(uint16 bobNum) const { return _personFrames[bobNum]; }

	void joeSetupFromBanks(const char *animBank, const char *standBank);

	//! SETUP_JOE(), loads the various bobs needed to animate Joe
	void joeSetup();

	//! SETUP_HERO(), places Joe at the right place when entering a room
	ObjectData *joeSetupInRoom(bool autoPosition, uint16 scale);
	
	//! FACE_JOE()
	uint16 joeFace();

	//! GRAB_JOE()
	void joeGrab(uint16 state, uint16 speed);

	//! GRAB_DIR
	void joeGrabDirection(StateGrab grab, uint16 speed);

	void joeUseDress(bool showCut);
	void joeUseClothes(bool showCut);
	void joeUseUnderwear();

	void joeSpeak(uint16 descNum, bool objectType = false);

	void makePersonSpeak(const char *sentence, Person *person, const char *voiceFilePrefix);
	void dialogue(const char *dlgFile, int personInRoom, char *cutaway);
	void playCutaway(const char *cutFile, char *next = NULL);

	const char *objectOrItemName(int16 obj) const;

	Verb findVerbUnderCursor(int16 cursorx, int16 cursory) const;
	uint16 findObjectUnderCursor(int16 cursorx, int16 cursory) const;

	Walk *walk() const { return _walk; }
	Display *display() const { return _display; }
	Command *command() const { return _cmd; }
	Resource *resource() const { return _resource; }

	uint16 findObjectRoomNumber(uint16 zoneNum) const;
	uint16 findObjectGlobalNumber(uint16 zoneNum) const;

	void inventorySetup();
	uint16 findInventoryItem(int invSlot) const;
	void inventoryRefresh();
	int16 previousInventoryItem(int16 start) const;
	int16 nextInventoryItem(int16 start) const;
	void removeDuplicateItems();
	uint16 numItemsInventory() const;
	void inventoryInsertItem(uint16 itemNum, bool refresh = true);
	void inventoryDeleteItem(uint16 itemNum, bool refresh = true);
	void inventoryScroll(uint16 count, bool up);

	//! Ugly hack from original code
	void sceneReset() { _scene = 0; }

	//! Make a scene
	void sceneStart();

	//! Stop making a scene
	void sceneStop();

	//! Copy data from dummy object to object
	void objectCopy(int dummyObjectIndex, int objectIndex);

	void checkPlayer();

	void customMoveJoe(int facing, uint16 areaNum, uint16 walkDataNum);

	void handlePinnacleRoom();

	void update();

	void useJournal();

	bool gameSave(uint16 slot, const char *desc);
	bool gameLoad(uint16 slot);

	enum {
		MAX_ZONES_NUMBER   = 32,
		MAX_AREAS_NUMBER   = 11,
		JOE_RESPONSE_MAX   = 40,
		DEFAULT_TALK_SPEED = 7,
		GAME_STATE_COUNT   = 211,
		TALK_SELECTED_COUNT = 86
	};

	static Common::RandomSource randomizer;

protected:

	void initialise();

	//! Contents of queen.jas file
	uint8 *_jas;

	uint16 _currentRoom;
	uint16 _oldRoom;
	uint16 _newRoom;

	//! Total number of room in game
	uint16 _numRooms;

	//! First object number in room
	uint16 *_roomData;

	//! Background music to play in room
	uint16 *_sfxName;

	//! Number of objects in room
	int16 *_objMax;

	//! Bounding box of object
	Box *_objectBox;

	//! Inventory items
	ItemData *_itemData;
	uint16 _numItems;

	GraphicData *_graphicData;
	uint16 _numGraphics;

	ObjectData *_objectData;
	uint16 _numObjects;

	ObjectDescription *_objectDescription;
	uint16 _numObjDesc;

	ActorData *_actorData;
	uint16 _numActors;	//ACTOR_DATA_MAX

	//! Areas in room
	Area (*_area)[MAX_AREAS_NUMBER];

	//! Number of areas in room
	int16 *_areaMax;

	//! Walk off point for an object
	WalkOffData *_walkOffData;
	uint16 _numWalkOffs;

	FurnitureData *_furnitureData;
	uint16 _numFurniture;	//FURN_DATA_MAX
	
	GraphicAnim *_graphicAnim;
	uint16 _numGraphicAnim;	//GRAPHIC_ANIM_MAX

	//! Current areas in room
	ZoneSlot _zones[2][MAX_ZONES_NUMBER];

	//! Actor position in room is _walkOffData[_entryObj]
	int16 _entryObj;

	//! Object description (Look At)
	char **_objDescription;	//OBJECT_DESCRstr
	uint16 _numDescriptions;

	char **_objName;	//OBJECT_NAMEstr
	uint16 _numNames;

	//! Room name, prefix for data files (PCX, LUM...)
	char **_roomName;	//ROOM_NAMEstr	

	char *_joeResponse[JOE_RESPONSE_MAX + 1];	//JOE_RESPstr

	//! Actor animation string
	char **_aAnim;
	uint16 _numAAnim;	//A_ANIM_MAX

	//! Actor name
	char **_aName;
	uint16 _numAName;	//A_NAME_MAX

	//! Actor filename
	char **_aFile;
	uint16 _numAFile;	//A_FILE_MAX

	struct {
		uint16 x, y;
		uint16 facing, cutFacing, prevFacing;
		JoeWalkMode walk;
		uint16 scale;
	} _joe;
	
	int16 _gameState[GAME_STATE_COUNT];

	TalkSelected _talkSelected[TALK_SELECTED_COUNT];
	
	//! Number of animated furniture in current room (FMAXA)
	uint16 _numFurnitureAnimated;

	//! Number of static furniture in current room (FMAX)
	uint16 _numFurnitureStatic;

	//! Total number of frames for the animated furniture (FMAXLEN)
	uint16 _numFurnitureAnimatedLen;

	//! Current number of frames unpacked (FRAMES)
	uint16 _numFrames;

	//! Last frame number used for person animation
	uint16 _personFrames[4];

	//! Describe a string based animation (30 frames maximum, bob number must be < 17)
	AnimFrame _newAnim[17][30];

	GameConfig _settings;

	//! Inventory items
	int16 _inventoryItem[4];

	//! scene counter
	int _scene;

	Resource *_resource;
	Graphics *_graphics;
	Debug *_dbg;
	Display *_display;
	Input *_input;
	Sound *_sound;
	Walk *_walk;
	Command *_cmd;

	//! Verbs (in order) available in panel
	static const VerbEnum PANEL_VERBS[];
};


} // End of namespace Queen

#endif
