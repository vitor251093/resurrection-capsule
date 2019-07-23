#ifndef __LOBBY_2_PRESENCE_H
#define __LOBBY_2_PRESENCE_H

#include "RakString.h"

namespace RakNet
{
	class BitStream;

	/// Lobby2Presence is information about your online status. It is only held in memory, so is lost when you go offline.
	/// Set by calling Client_SetPresence and retrieve with Client_GetPresence
	struct Lobby2Presence
	{
		Lobby2Presence();
		~Lobby2Presence();
		void Serialize(RakNet::BitStream *bitStream, bool writeToBitstream);

		enum Status
		{
			/// Set by the constructor, meaning it was never set. This is the default if a user is online, but SetPresence() was never called.
			UNDEFINED,
			/// Returned by Client_GetPresence() if you query for a user that is not online, whether or not SetPresence was ever called().
			NOT_ONLINE,
			/// Set by the user (you)
			AWAY,
			/// Set by the user (you)
			DO_NOT_DISTURB,
			/// Set by the user (you)
			MINIMIZED,
			/// Set by the user (you)
			TYPING,
			/// Set by the user (you)
			VIEWING_PROFILE,
			/// Set by the user (you)
			EDITING_PROFILE,
			/// Set by the user (you)
			IN_LOBBY,
			/// Set by the user (you)
			IN_ROOM,
			/// Set by the user (you)
			IN_GAME
		} status;

		/// Visibility flag. This is not enforced by the server, so if you want a user's presence to be not visible, just don't display it on the client
		bool isVisible;

		/// Although game name is also present in the titleName member of Client_Login, this is the visible name returned by presence queries
		/// That is because Client_Login::titleName member is optional, for example for lobbies that support multiple titles.
		/// Set by the user (you) or leave blank if desired.
		RakString titleName;

		/// Anything you want
		RakString statusString;
	};
}

#endif
