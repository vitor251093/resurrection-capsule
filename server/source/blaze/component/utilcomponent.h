
#ifndef _BLAZE_COMPONENT_UTIL_HEADER
#define _BLAZE_COMPONENT_UTIL_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// UtilComponent
	class UtilComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Response
			static void SendPostAuth(Client* client, Header header);
			static void SendGetTickerServer(Client* client);
			static void SendUserOptions(Client* client);

		private:
			static void FetchClientConfig(Client* client, Header header);
			static void Ping(Client* client, Header header);
			static void GetTelemetryServer(Client* client, Header header);
			static void PreAuth(Client* client, Header header);
			static void PostAuth(Client* client, Header header);
			static void UserSettingsSave(Client* client, Header header);
			static void UserSettingsLoadAll(Client* client, Header header);
			static void SetClientMetrics(Client* client, Header header);
	};
}

#endif
