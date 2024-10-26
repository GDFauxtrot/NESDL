#include "NESDL.h"

void NESDL_Input::Init(NESDL_Core* c)
{
    core = c;
    player1 = {0}; // Initialize values to 0
    player2 = {0}; // Initialize values to 0
    
    readBit = 0; // Start "unloaded", AKA all reads will return a default (1)
}

void NESDL_Input::SetControllerConnected(bool connected, bool isPlayer2)
{
    if (ignoreChanges)
    {
        return;
    }
    
    if (!isPlayer2)
    {
        player1.connected = connected;
    }
    else
    {
        player2.connected = connected;
    }
}

void NESDL_Input::RegisterKey(SDL_KeyCode keyCode, bool keyDown)
{
    if (ignoreChanges)
    {
        return;
    }
    
    // Set key values for controllers even if they're marked as "disconnected"
    // TODO hard-coded keys for now! While we're setting things up
    switch (keyCode)
    {
        case SDLK_w:
            player1.up = keyDown;
            break;
        case SDLK_s:
            player1.down = keyDown;
            break;
        case SDLK_a:
            player1.left = keyDown;
            break;
        case SDLK_d:
            player1.right = keyDown;
            break;
        case SDLK_n:
            player1.b = keyDown;
            break;
        case SDLK_m:
            player1.a = keyDown;
            break;
        case SDLK_COMMA:
            player1.select = keyDown;
            break;
        case SDLK_PERIOD:
        case SDLK_ESCAPE:
            player1.start = keyDown;
            break;
        // No player 2 -- yet
        default:
            break;
    }
}

// Unused function?
uint8_t NESDL_Input::PlayerInputToByte(bool isPlayer2)
{
    uint8_t result = 0;
    PlayerInput player = isPlayer2 ? player2 : player1;
    
    if (player.connected)
    {
        result |= (player.a         << 0);
        result |= (player.b         << 1);
        result |= (player.select    << 2);
        result |= (player.start     << 3);
        result |= (player.up        << 4);
        result |= (player.down      << 5);
        result |= (player.left      << 6);
        result |= (player.right     << 7);
    }
    
    return result;
}

void NESDL_Input::SetReadInputStrobe(bool strobe)
{
    if (ignoreChanges)
    {
        return;
    }
    
    if (readInputStrobe == true && strobe == false)
    {
        readBit = 0;
    }
    
    readInputStrobe = strobe;
}

bool NESDL_Input::GetNextPlayerInputBit(bool isPlayer2)
{
    PlayerInput player = isPlayer2 ? player2 : player1;
    
    if (player.connected)
    {
        bool result = GetPlayerInputBit(readInputStrobe == true ? 0 : readBit, isPlayer2);
        if (!ignoreChanges)
        {
            readBit++;
        }
        return result;
    }
    else
    {
        return false;
    }
}

bool NESDL_Input::GetPlayerInputBit(uint8_t index, bool isPlayer2)
{
    PlayerInput player = isPlayer2 ? player2 : player1;
    
    switch (index)
    {
        case 0:
            return player.a;
        case 1:
            return player.b;
        case 2:
            return player.select;
        case 3:
            return player.start;
        case 4:
            return player.up;
        case 5:
            return player.down;
        case 6:
            return player.left;
        case 7:
            return player.right;
    }
    
    return 1;
}
