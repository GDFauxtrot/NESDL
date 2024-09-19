#pragma once

struct PlayerInput
{
    bool connected;
    bool up;
    bool down;
    bool left;
    bool right;
    bool a;
    bool b;
    bool select;
    bool start;
};

class NESDL_Input
{
public:
	void Init(NESDL_Core* c);
    void RegisterKey(SDL_KeyCode keyCode, bool keyDown);
    void SetControllerConnected(bool connected, bool isPlayer2);
    uint8_t PlayerInputToByte(bool isPlayer2);
    bool GetNextPlayerInputBit(bool isPlayer2);
    void SetReadInputStrobe(bool strobe);
    bool ignoreChanges;
private:
    bool GetPlayerInputBit(uint8_t index, bool isPlayer2);
    
    NESDL_Core* core;
    PlayerInput player1;
    PlayerInput player2;
    uint8_t readBit;
    bool readInputStrobe;
};
