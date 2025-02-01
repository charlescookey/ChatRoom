#pragma once

#define _USE_MATH_DEFINES
#include <fmod.hpp>
#include <fmod_errors.h>
#include <conio.h>
#pragma comment(lib, "fmod_vc.lib")

class Sound {
	FMOD::System* system;

	FMOD::Sound* sound1 = NULL;
	FMOD::Channel* channel1 = NULL;

	FMOD::Sound* sound2 = NULL;
	FMOD::Channel* channel2 = NULL;


public:
	Sound() {
		FMOD::System_Create(&system);
		system->init(512, FMOD_INIT_NORMAL, NULL);

		system->createSound("pm.mp3", FMOD_DEFAULT, NULL, &sound1);
		sound1->setMode(FMOD_LOOP_OFF);

		system->createSound("gm.mp3", FMOD_DEFAULT, NULL, &sound2);
		sound2->setMode(FMOD_LOOP_OFF);
	}
	
	~Sound() {
		sound2->release();
		sound1->release();
		system->close();
		system->release();
	}

	void playPMSound() {
		system->playSound(sound1, NULL, false, &channel1);
	}
	
	void playGMSound() {
		system->playSound(sound2, NULL, false, &channel2);
	}
};


//if (key == 32) {
//	bool isPlaying = false;
//	channel2->isPlaying(&isPlaying);
//	if (isPlaying == false) {
//		system->playSound(sound2, NULL, false, &channel2);
//	}
//}

