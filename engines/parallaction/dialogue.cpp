/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/stdafx.h"

#include "common/events.h"
#include "parallaction/parallaction.h"



namespace Parallaction {

#define SKIPPED_ANSWER		   1000

#define MAX_BALLOON_WIDTH			130

#define MAX_PASSWORD_LENGTH 		 7

#define QUESTION_BALLOON_X			140
#define QUESTION_BALLOON_Y			10
#define QUESTION_CHARACTER_X		  190
#define QUESTION_CHARACTER_Y		  80

#define ANSWER_CHARACTER_X			10
#define ANSWER_CHARACTER_Y			80

int16 selectAnswer(Question *q, StaticCnv*);
int16 getHoverAnswer(int16 x, int16 y, Question *q);

int16 _answerBalloonX[10] = { 80, 120, 150, 150, 150, 0, 0, 0, 0, 0 };
int16 _answerBalloonY[10] = { 10, 70, 130, 0, 0, 0, 0, 0, 0, 0 };
int16 _answerBalloonW[10] = { 0 };
int16 _answerBalloonH[10] = { 0 };



Dialogue *Parallaction::parseDialogue(Script &script) {
//	printf("parseDialogue()\n");
	uint16 num_questions = 0;
	uint16 v50[20];
	Table _questions_names(20);
	Question *_questions[20];

	for (uint16 _si = 0; _si < 20; _si++) {
		v50[_si] = 0;
	}

	fillBuffers(script, true);

	while (scumm_stricmp(_tokens[0], "enddialogue")) {
		if (scumm_stricmp(_tokens[0], "Question")) continue;

		_questions[num_questions] = new Dialogue;
		Dialogue *vB4 = _questions[num_questions];

		_questions_names.addData(_tokens[1]);

		vB4->_text = parseDialogueString(script);
//		printf("Question: '%s'\n", vB4->_text);

		fillBuffers(script, true);
		vB4->_mood = atoi(_tokens[0]);

		uint16 _di = 0;

		fillBuffers(script, true);
		while (scumm_stricmp(_tokens[0], "endquestion")) {	// parse answers

			vB4->_answers[_di] = new Answer;

			if (_tokens[1][0]) {

				Table* v60 = _localFlagNames;
				uint16 v56 = 1;

				if (!scumm_stricmp(_tokens[1], "global")) {
					v56 = 2;
					v60 = _globalTable;
					vB4->_answers[_di]->_yesFlags |= kFlagsGlobal;
				}

				do {

					if (!scumm_strnicmp(_tokens[v56], "no", 2)) {
						byte _al = v60->lookup(_tokens[v56]+2);
						vB4->_answers[_di]->_noFlags |= 1 << (_al - 1);
					} else {
						byte _al = v60->lookup(_tokens[v56]);
						vB4->_answers[_di]->_yesFlags |= 1 << (_al - 1);
					}

					v56++;

				} while (!scumm_stricmp(_tokens[v56++], "|"));

			}

			vB4->_answers[_di]->_text = parseDialogueString(script);

//			printf("answer[%i]: '%s'\n", _di, vB4->_answers[_di]);

			fillBuffers(script, true);
			vB4->_answers[_di]->_mood = atoi(_tokens[0]);
			vB4->_answers[_di]->_following._name = parseDialogueString(script);

			fillBuffers(script, true);
			if (!scumm_stricmp(_tokens[0], "commands")) {
				parseCommands(script, vB4->_answers[_di]->_commands);
				fillBuffers(script, true);
			}

			_di++;
		}

		fillBuffers(script, true);
		num_questions++;

	}

	for (uint16 _si = 0; _si <num_questions; _si++) {

		for (uint16 v5A = 0; v5A < 5; v5A++) {
			if (_questions[_si]->_answers[v5A] == 0) continue;

			int16 v58 = _questions_names.lookup(_questions[_si]->_answers[v5A]->_following._name);
			free(_questions[_si]->_answers[v5A]->_following._name);

			if (v58 == -1) {
				_questions[_si]->_answers[v5A]->_following._question = 0;
			} else {
				_questions[_si]->_answers[v5A]->_following._question = _questions[v58-1];

				if (v50[v58]) {
					_questions[_si]->_answers[v5A]->_mood |= 0x10;
				}

				v50[v58] = 1;
			}
		}
	}

	return _questions[0];
}


char *Parallaction::parseDialogueString(Script &script) {

	char vC8[200];
	char *vD0 = NULL;
	do {

		vD0 = script.readLine(vC8, 200);
		if (vD0 == 0) return NULL;

		vD0 = Common::ltrim(vD0);

	} while (strlen(vD0) == 0);

	vD0[strlen(vD0)-1] = '\0';	// deletes the trailing '0xA'
								// this is critical for Gfx::displayWrappedString to work properly

	char *vCC = (char*)malloc(strlen(vD0)+1);
	strcpy(vCC, vD0);

	return vCC;
}

class DialogueManager {

	Parallaction	*_vm;
	SpeakData		*_data;
	Dialogue		*_dialogue;

	bool 			_askPassword;

	bool 			isNpc;
	Cnv				*_questioner;
	Cnv				*_answerer;

	Question		*_q;

public:
	DialogueManager(Parallaction *vm, SpeakData *data) : _vm(vm), _data(data) {
		_dialogue = _data->_dialogue;
		isNpc = scumm_stricmp(_data->_name, "yourself") && _data->_name[0] != '\0';
		_questioner = isNpc ? _vm->_disk->loadTalk(_data->_name) : _vm->_char._talk;
		_answerer = _vm->_char._talk;
	}

	~DialogueManager() {
		if (isNpc) {
			delete _questioner;
		}
	}

	void run();

protected:
	void clear() {
		_vm->_gfx->copyScreen(Gfx::kBitBack, Gfx::kBitFront);
	}

	void displayQuestion();
	bool displayAnswers();
	bool displayAnswer(uint16 i);

	uint16 getAnswer();
	int16 selectAnswer();
	uint16 askPassword();
	int16 getHoverAnswer(int16 x, int16 y);

};

uint16 DialogueManager::askPassword() {
	debugC(1, kDebugDialogue, "checkDialoguePassword()");

	char password[100];
	uint16 passwordLen = 0;

	while (true) {
		clear();

		strcpy(password, ".......");

		Common::Rect r(_answerBalloonW[0], _answerBalloonH[0]);
		r.moveTo(_answerBalloonX[0], _answerBalloonY[0]);

		_vm->_gfx->drawBalloon(r, 1);
		_vm->_gfx->displayWrappedString(_q->_answers[0]->_text, _answerBalloonX[0], _answerBalloonY[0], 3, MAX_BALLOON_WIDTH);
		_vm->_gfx->flatBlitCnv(_answerer, 0, ANSWER_CHARACTER_X, ANSWER_CHARACTER_Y,	Gfx::kBitFront);
		_vm->_gfx->displayString(_answerBalloonX[0] + 5, _answerBalloonY[0] + _answerBalloonH[0] - 15, "> ", 0);
		_vm->_gfx->updateScreen();

		Common::Event e;
		while (e.kbd.ascii != Common::KEYCODE_RETURN && passwordLen < MAX_PASSWORD_LENGTH) {

			// FIXME: see comment for updateInput()
			if (!g_system->getEventManager()->pollEvent(e)) continue;
			if (e.type != Common::EVENT_KEYDOWN) continue;
			if (e.type == Common::EVENT_QUIT)
				g_system->quit();
			if (!isdigit(e.kbd.ascii)) continue;

			password[passwordLen] = e.kbd.ascii;
			passwordLen++;
			password[passwordLen] = '\0';

			_vm->_gfx->displayString(_answerBalloonX[0] + 10, _answerBalloonY[0] + _answerBalloonH[0] - 15, password, 0);
			_vm->_gfx->updateScreen();

			g_system->delayMillis(20);
		}

		if ((!scumm_stricmp(_vm->_characterName, _doughName) && !scumm_strnicmp(password, "1732461", 7)) ||
			(!scumm_stricmp(_vm->_characterName, _donnaName) && !scumm_strnicmp(password, "1622", 4)) ||
			(!scumm_stricmp(_vm->_characterName, _dinoName) && !scumm_strnicmp(password, "179", 3))) {

			break;

		}

	}

	return 0;

}



bool DialogueManager::displayAnswer(uint16 i) {

	uint32 v28 = _localFlags[_vm->_currentLocationIndex];
	if (_q->_answers[i]->_yesFlags & kFlagsGlobal)
		v28 = _commandFlags | kFlagsGlobal;

	// display suitable answers
	if (((_q->_answers[i]->_yesFlags & v28) == _q->_answers[i]->_yesFlags) && ((_q->_answers[i]->_noFlags & ~v28) == _q->_answers[i]->_noFlags)) {

		_vm->_gfx->getStringExtent(_q->_answers[i]->_text, MAX_BALLOON_WIDTH, &_answerBalloonW[i], &_answerBalloonH[i]);

		Common::Rect r(_answerBalloonW[i], _answerBalloonH[i]);
		r.moveTo(_answerBalloonX[i], _answerBalloonY[i]);

		_vm->_gfx->drawBalloon(r, 1);

		_answerBalloonY[i+1] = 10 + _answerBalloonY[i] + _answerBalloonH[i];
		_askPassword = _vm->_gfx->displayWrappedString(_q->_answers[i]->_text, _answerBalloonX[i], _answerBalloonY[i], 3, MAX_BALLOON_WIDTH);

		return true;
	}

	_answerBalloonY[i+1] = _answerBalloonY[i];
	_answerBalloonY[i] = SKIPPED_ANSWER;

	return false;

}

bool DialogueManager::displayAnswers() {

	bool displayed = false;

	uint16 i = 0;

	while (i < NUM_ANSWERS && _q->_answers[i]) {
		if (displayAnswer(i))
			displayed = true;

		i++;
	}
	_vm->_gfx->updateScreen();

	return displayed;
}

void DialogueManager::displayQuestion() {

	int16 w = 0, h = 0;

	if (!scumm_stricmp(_q->_text, "NULL")) return;

	_vm->_gfx->flatBlitCnv(_questioner, _q->_mood & 0xF, QUESTION_CHARACTER_X, QUESTION_CHARACTER_Y, Gfx::kBitFront);
	_vm->_gfx->getStringExtent(_q->_text, MAX_BALLOON_WIDTH, &w, &h);

	Common::Rect r(w, h);
	r.moveTo(QUESTION_BALLOON_X, QUESTION_BALLOON_Y);

	_vm->_gfx->drawBalloon(r, _q->_mood & 0x10);
	_vm->_gfx->displayWrappedString(_q->_text, QUESTION_BALLOON_X, QUESTION_BALLOON_Y, 0, MAX_BALLOON_WIDTH);
	_vm->_gfx->updateScreen();

	waitUntilLeftClick();

	clear();

	return;
}

uint16 DialogueManager::getAnswer() {

	uint16 answer = 0;

	if (_askPassword == false) {
		answer = selectAnswer();
	} else {
		answer = askPassword();
	}

	clear();

	debugC(1, kDebugDialogue, "runDialogue: user selected answer #%i", answer);

	return answer;
}

void DialogueManager::run() {

	_askPassword = false;
	CommandList *cmdlist = NULL;

	_q = _dialogue;
	int16 answer;

	while (_q) {

		answer = 0;

		displayQuestion();
		if (_q->_answers[0] == NULL) break;

		_answerBalloonY[0] = 10;

		if (scumm_stricmp(_q->_answers[0]->_text, "NULL")) {
			if (!displayAnswers()) break;
			answer = getAnswer();
			cmdlist = &_q->_answers[answer]->_commands;
		}

		_q = _q->_answers[answer]->_following._question;
	}

	clear();

	if (cmdlist)
		_vm->runCommands(*cmdlist);

}

int16 DialogueManager::selectAnswer() {

	int16 numAvailableAnswers = 0;
	int16 _si = 0;
	int16 _di = 0;

	int16 i = 0;
	for (; _q->_answers[i]; i++) {
		if (_answerBalloonY[i] == SKIPPED_ANSWER) continue;

		_di = i;
		numAvailableAnswers++;
	}
	_answerBalloonY[i] = 2000;

	if (numAvailableAnswers == 1) {
		_vm->_gfx->displayWrappedString(_q->_answers[_di]->_text, _answerBalloonX[_di], _answerBalloonY[_di], 0, MAX_BALLOON_WIDTH);
		_vm->_gfx->flatBlitCnv(_answerer, 0, ANSWER_CHARACTER_X,	ANSWER_CHARACTER_Y, Gfx::kBitFront);
		_vm->_gfx->updateScreen();
		waitUntilLeftClick();
		return _di;
	}

	int16 v2 = -1;

	_mouseButtons = kMouseNone;
	while (_mouseButtons != kMouseLeftUp) {

		_vm->updateInput();
		_si = getHoverAnswer(_vm->_mousePos.x, _vm->_mousePos.y);

		if (_si != v2) {
			if (v2 != -1)
				_vm->_gfx->displayWrappedString(_q->_answers[v2]->_text, _answerBalloonX[v2], _answerBalloonY[v2], 3, MAX_BALLOON_WIDTH);

			_vm->_gfx->displayWrappedString(_q->_answers[_si]->_text, _answerBalloonX[_si],	_answerBalloonY[_si], 0, MAX_BALLOON_WIDTH);
			_vm->_gfx->flatBlitCnv(_answerer, _q->_answers[_si]->_mood & 0xF, ANSWER_CHARACTER_X, ANSWER_CHARACTER_Y, Gfx::kBitFront);
		}

		_vm->_gfx->updateScreen();
		g_system->delayMillis(30);
		v2 = _si;
	}

	return _si;
}


//
//	finds out which answer is currently selected
//
int16 DialogueManager::getHoverAnswer(int16 x, int16 y) {

	int16 top = 1000;
	int16 bottom = 1000;

	for (int16 _si = 0; _si < NUM_ANSWERS; _si++) {
		if (_q->_answers[_si] == NULL) break;

		if (_answerBalloonY[_si] != SKIPPED_ANSWER) {
			top = _answerBalloonY[_si];
		}

		int16 _di = _si + 1;
		for (; _answerBalloonY[_di] == SKIPPED_ANSWER; _di++) ;

		bottom = _answerBalloonY[_di];

		// mouse position is compared only with y coordinates
		if (y > top && y < bottom) return _si;

	}

	return 0;

}



void Parallaction::runDialogue(SpeakData *data) {
	debugC(1, kDebugDialogue, "runDialogue: starting dialogue '%s'", data->_name);

	_gfx->setFont(kFontDialogue);
	showCursor(false);

	DialogueManager man(this, data);
	man.run();

	refreshInventory(_characterName);
	showCursor(true);

	return;
}

Answer::Answer() {
	_text = NULL;
	_mood = 0;
	_following._question =  NULL;
	_noFlags = 0;
	_yesFlags = 0;
}

Answer::~Answer() {
	if (_mood & 0x10)
		delete _following._question;

	if (_text)
		free(_text);

}

Question::Question() {
	_text = NULL;
	_mood = 0;

	for (uint32 i = 0; i < NUM_ANSWERS; i++)
		_answers[i] = NULL;

}

Question::~Question() {

	for (uint32 i = 0; i < NUM_ANSWERS; i++)
		if (_answers[i]) delete _answers[i];

	free(_text);
}

} // namespace Parallaction
