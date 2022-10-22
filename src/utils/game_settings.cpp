
#include "game_settings.h"

static GameSettings gs;

GameSettings& GameSettings::get()
{
	return gs;
}
