
// Include
#include "api.h"
#include "config.h"

#include "../main.h"

#include "../http/session.h"
#include "../http/router.h"
#include "../http/uri.h"
#include "../http/multipart.h"
#include "../utils/functions.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/beast/version.hpp>

#include <iostream>
#include <filesystem>

/*
	api.account.auth
		response
			timestamp
				{TIMESTAMP}

			account
				tutorial_completed
				chain_progression
				creature_rewards
				current_game_id
				current_playgroup_id
				default_deck_pve_id
				default_deck_pvp_id
				level
				avatar_id
				id
				new_player_inventory
				new_player_progress
				cashout_bonus_time
				star_level
				unlock_catalysts
				unlock_diagonal_catalysts
				unlock_fuel_tanks
				unlock_inventory
				unlock_pve_decks
				unlock_pvp_decks
				unlock_stats
				unlock_inventory_identify
				unlock_editor_flair_slots
				upsell
				xp
				grant_all_access
				cap_level
				cap_progression

			creatures
				id
				name
				png_thumb_url
				noun_id
				version
				gear_score
				item_points

			decks
				name
				category
				id
				slot
				locked
				creatures
					{SAME_AS_CREATURES_BEFORE}

			feed
				items

			settings

			server_tuning
				itemstore_offer_period
				itemstore_current_expiration
				itemstore_cost_multiplier_basic
				itemstore_cost_multiplier_uncommon
				itemstore_cost_multiplier_rare
				itemstore_cost_multiplier_epic
				itemstore_cost_multiplier_unique
				itemstore_cost_multiplier_rareunique
				itemstore_cost_multiplier_epicunique

*/

// XML Writer
struct xml_string_writer : pugi::xml_writer {
	std::string result;

	void write(const void* data, size_t size) override {
		result.append(static_cast<const char*>(data), size);
	}
};

// Game
namespace Game {
	constexpr std::string_view skipLauncherScript = R"(
<!DOCTYPE html PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html>
	<head>
		<script type="text/javascript">
			window.onload = function() {
				Client.playCurrentApp();
			}
		</script>
	</head>
	<body>
	</body>
</html>
)";

	constexpr std::string_view dlsClientScript = R"(
<script>
	var DLSClient = {};
	DLSClient.getRequest = function(url, callback) {
		var xmlHttp = new XMLHttpRequest(); 
		xmlHttp.onload = function() {
			callback(xmlHttp.responseText);
		}
		xmlHttp.open("GET", url, true);
		xmlHttp.send(null);
	};
	DLSClient.request = function(name, params, callback) {
		if (params !== undefined && typeof params === 'object') {
			var str = [];
			for (var p in params)
				if (params.hasOwnProperty(p)) {
					str.push(encodeURIComponent(p) + "=" + encodeURIComponent(params[p]));
				}
			params = str.join("&");
		}
		DLSClient.getRequest("http://{{host}}/dls/api?method=" + name + (params === undefined ? "" : ("&" + params)), callback);
	};
</script>
)";

	// API
	API::API(const std::string& version) : mVersion(version) {
		// Empty
	}

	void API::responseWithFileInStorage(HTTP::Session& session, HTTP::Response& response) {
		responseWithFileInStorage(session, response, "");
	}

	void API::responseWithFileInStorage(HTTP::Session& session, HTTP::Response& response, std::string path) {
		auto& request = session.get_request();
		std::string name = request.uri.resource();
		if (name.ends_with("/")) name = name + "index.html";
		responseWithFileInStorageAtPath(session, response, path + name);
	}

	void API::responseWithFileInStorageAtPath(HTTP::Session& session, HTTP::Response& response, std::string path) {
		std::string wholePath = Config::Get(CONFIG_STORAGE_PATH) + path;

		response.version() |= 0x1000'0000;
		response.body() = std::move(wholePath);
	}

	void API::setup() {
		const auto& router = Application::GetApp().get_http_server()->get_router();

		// Routing
		router->add("/api", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [](HTTP::Session& session, HTTP::Response& response) {
			std::cout << "Got API route." << std::endl;
		});

		// DLS
		router->add("/dls/api", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			auto method = request.uri.parameter("method");
			if (method == "api.launcher.setTheme") {
				dls_launcher_setTheme(session, response);
			} else if (method == "api.launcher.listThemes") {
				dls_launcher_listThemes(session, response);
			} else if (method == "api.game.registration") {
				dls_game_registration(session, response);
			} else {
				response.result() = boost::beast::http::status::internal_server_error;
			}
		});

		// Launcher
		router->add("/bootstrap/api", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			auto method = request.uri.parameter("method");
			if (method == "api.config.getConfigs") {
				bootstrap_config_getConfig(session, response);
				return;
			}

			response.result() = boost::beast::http::status::internal_server_error;
		});

		router->add("/bootstrap/launcher/", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			auto version = request.uri.parameter("version");
			if (Config::GetBool(CONFIG_SKIP_LAUNCHER)) {
				response.set(boost::beast::http::field::content_type, "text/html");
				response.body() = skipLauncherScript;
			} else {
				std::string path = Config::Get(CONFIG_STORAGE_PATH) +
					"www/" +
					Config::Get(CONFIG_DARKSPORE_LAUNCHER_THEMES_PATH) +
					mActiveTheme + "/index.html";

				std::string client_script(dlsClientScript);
				utils::string_replace(client_script, "{{host}}", Config::Get(CONFIG_SERVER_HOST));

				std::string file_data = utils::get_file_text(path);
				utils::string_replace(file_data, "</head>", client_script + "</head>");

				response.set(boost::beast::http::field::content_type, "text/html");
				response.body() = std::move(file_data);
			}
		});

		router->add("/bootstrap/launcher/images/([a-zA-Z0-9_.]+)", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			const std::string& resource = request.uri.resource();

			std::string path = Config::Get(CONFIG_STORAGE_PATH) +
				"www/" +
				Config::Get(CONFIG_DARKSPORE_LAUNCHER_THEMES_PATH) +
				mActiveTheme + "/images/" +
				resource.substr(resource.rfind('/') + 1);

			response.version() |= 0x1000'0000;
			response.body() = std::move(path);
		});

		router->add("/bootstrap/launcher/notes", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			std::string path = Config::Get(CONFIG_STORAGE_PATH) +
				"www/" +
				Config::Get(CONFIG_DARKSPORE_LAUNCHER_NOTES_PATH);

			std::string file_data = utils::get_file_text(path);
			utils::string_replace(file_data, "{{dls-version}}", "0.1");
			utils::string_replace(file_data, "{{version-lock}}", Config::GetBool(CONFIG_VERSION_LOCKED) ? "5.3.0.127" : "no");
			utils::string_replace(file_data, "{{game-mode}}", Config::GetBool(CONFIG_SINGLEPLAYER_ONLY) ? "singleplayer" : "multiplayer");
			utils::string_replace(file_data, "{{display-latest-version}}", "none");
			utils::string_replace(file_data, "{{latest-version}}", "yes");

			response.set(boost::beast::http::field::content_type, "text/html");
			response.body() = std::move(file_data);
		});

		// Game
		router->add("/game/api", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			if (request.data.method() == boost::beast::http::verb::post) {
				// Fetch boundary later from request.data[boost::beast::http::field::content_type]
				
				HTTP::Multipart multipart(request.data.body(), "EA_HTTP_REQUEST_SIMPLE_BOUNDARY");
				for (const auto&[name, value] : multipart) {
					request.uri.set_parameter(name, value);
				}
			}

			auto cookies = request.data[boost::beast::http::field::cookie];
			if (!cookies.empty()) {
				auto cookieList = boost::beast::http::param_list(";" + cookies.to_string());
				for (auto& [name, value] : request.uri) {
					if (value == "cookie") {
						for (const auto& param : cookieList) {
							if (std::strncmp(param.first.data(), name.c_str(), name.length()) == 0) {
								value = param.second.to_string();
								break;
							}
						}
					}
					std::cout << name << " = " << value << std::endl;
				}
				std::cout << std::endl;
			}

			auto version = request.uri.parameter("version");
			auto build = request.uri.parameter("build");

			auto method = request.uri.parameter("method");
			if (method.empty()) {
				if (request.uri.parameter("token") == "cookie") {
					method = "api.account.auth";
				} else {
					method = "api.account.getAccount";
				}
			}

			auto token = request.uri.parameter("token");
			if (!token.empty()) {
				const auto& user = Game::UserManager::GetUserByAuthToken(token);
				if (user) {
					session.set_user(user);
				}
			}

			if (method.empty()) {
				if (request.uri.parameter("token") == "cookie") {
					method = "api.account.auth";
				} else {
					method = "api.account.getAccount";
				}
			}

			if (method == "api.account.setNewPlayerStats") {
				method = "api.account.auth";
			}

			if (method == "api.status.getStatus") {
				game_status_getStatus(session, response);
			} else if (method == "api.status.getBroadcastList") {
				game_status_getBroadcastList(session, response);
			} else if (method == "api.inventory.getPartList") {
				game_inventory_getPartList(session, response);
			} else if (method == "api.inventory.getPartOfferList") {
				game_inventory_getPartOfferList(session, response);
			} else if (method == "api.account.auth") {
				game_account_auth(session, response);
			} else if (method == "api.account.getAccount") {
				game_account_getAccount(session, response);
			} else if (method == "api.account.logout") {
				game_account_logout(session, response);
			} else if (method == "api.account.unlock") {
				game_account_unlock(session, response);
			} else if (method == "api.game.getGame" || method == "api.game.getRandomGame") {
				game_game_getGame(session, response);
			} else if (method == "api.game.exitGame") {
				game_game_exitGame(session, response);
			} else if (method == "api.creature.resetCreature") {
				game_creature_resetCreature(session, response);
			} else if (method == "api.creature.unlockCreature") {
				game_creature_unlockCreature(session, response);
			} else {
				std::cout << "Undefined /game/api method: " << method << std::endl;
				for (const auto& [name, value] : request.uri) {
					std::cout << name << " = " << value << std::endl;
				}
				std::cout << std::endl;
				empty_xml_response(response);
			}
		});

		/*
Undefined /game/api method: api.deck.updateDecks
method = api.deck.updateDecks
pve_active_slot = 1
pve_creatures = 10,11,3948469269,0,0,0,0,0,0
pvp_active_slot = 28614456
pvp_creatures = 0,0,0,0,0,0,0,0,0
token = ABCDEFGHIJKLMNOPQRSTUVWXYZ
version = 1
		*/



		// Png
		router->add("/template_png/([a-zA-Z0-9_.]+)", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			responseWithFileInStorage(session, response);
		});

		router->add("/creature_png/([a-zA-Z0-9_.]+)", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			responseWithFileInStorage(session, response);
		});


		// Browser launcher
		router->add("/favicon.ico", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			responseWithFileInStorage(session, response, "/www");
		});

		router->add("/panel/([/a-zA-Z0-9_.]*)", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			responseWithFileInStorage(session, response, "/www");
		});


		// Survey
		router->add("/survey/api", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			auto version = request.uri.parameter("version");
			auto method = request.uri.parameter("method");

			if (method == "api.survey.getSurveyList") {
				survey_survey_getSurveyList(session, response);
			} else {
				empty_xml_response(response);
			}
		});

		// Web
		router->add("/web/sporelabsgame/announceen", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			response.set(boost::beast::http::field::content_type, "text/html");
			response.body() = "<html><head><title>x</title></head><body>Announces</body></html>";
		});

		router->add("/web/sporelabsgame/register", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			std::string path = Config::Get(CONFIG_STORAGE_PATH) +
				"www/" +
				Config::Get(CONFIG_DARKSPORE_REGISTER_PAGE_PATH);

			std::string client_script(dlsClientScript);
			utils::string_replace(client_script, "{{host}}", Config::Get(CONFIG_SERVER_HOST));

			std::string file_data = utils::get_file_text(path);
			utils::string_replace(file_data, "</head>", client_script + "</head>");

			response.set(boost::beast::http::field::content_type, "text/html");
			response.body() = std::move(file_data);
		});

		router->add("/web/sporelabsgame/persona", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			response.set(boost::beast::http::field::content_type, "text/html");
			response.body() = "";
		});

		// QOS
		router->add("/qos/qos", { boost::beast::http::verb::get, boost::beast::http::verb::post }, [this](HTTP::Session& session, HTTP::Response& response) {
			auto& request = session.get_request();

			auto version = request.uri.parameteri("vers");
			auto type = request.uri.parameteri("qtyp");
			auto port = request.uri.parameteri("prpt");

			if (type == 1 && port == 3659) {
				pugi::xml_document document;

				auto docResponse = document.append_child("response");
				utils::xml_add_text_node(docResponse, "result", "1,0,1");
				add_common_keys(docResponse);

				xml_string_writer writer;
				document.save(writer, "\t", 1U, pugi::encoding_latin1);

				response.set(boost::beast::http::field::content_type, "text/xml");
				response.body() = std::move(writer.result);
			} else {
				response.set(boost::beast::http::field::content_type, "text/plain");
				response.body() = "";
			}
		});
	}

	void API::empty_xml_response(HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::empty_json_response(HTTP::Response& response) {
		response.set(boost::beast::http::field::content_type, "application/json");
	}

	void API::dls_launcher_setTheme(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();
		auto themeName = request.uri.parameter("theme");

		mActiveTheme = themeName;
		
		rapidjson::StringBuffer buffer;
		buffer.Clear();

		rapidjson::Document document;
		document.SetObject();

		// stat
		document.AddMember(rapidjson::Value("stat"), rapidjson::Value("ok"), document.GetAllocator());

		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);

		response.set(boost::beast::http::field::content_type, "application/json");
		response.body() = buffer.GetString();
	}

	void API::dls_launcher_listThemes(HTTP::Session& session, HTTP::Response& response) {
		rapidjson::StringBuffer buffer;
		buffer.Clear();

		rapidjson::Document document;
		document.SetObject();

		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

		// stat
		document.AddMember(rapidjson::Value("stat"), rapidjson::Value("ok"), allocator);
		
		// themes
		{
			std::string themesFolderPath = Config::Get(CONFIG_STORAGE_PATH) +
					"www/" + Config::Get(CONFIG_DARKSPORE_LAUNCHER_THEMES_PATH);
			rapidjson::Value value(rapidjson::kArrayType);
			for (const auto & entry : std::filesystem::directory_iterator(themesFolderPath)) {
				if (entry.is_directory()) {
					value.PushBack(rapidjson::Value{}.SetString(entry.path().filename().string().c_str(), 
														    	entry.path().filename().string().length(), allocator), allocator);
				}
			}

			document.AddMember(rapidjson::Value("themes"), value, allocator);
		}

		// selectedTheme
		document.AddMember(rapidjson::Value("selectedTheme"), 
				rapidjson::Value{}.SetString(mActiveTheme.c_str(), mActiveTheme.length(), allocator), allocator);

		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);

		response.set(boost::beast::http::field::content_type, "application/json");
		response.body() = buffer.GetString();
	}

	void API::dls_game_registration(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();
		auto name = request.uri.parameter("name");
		auto mail = request.uri.parameter("mail");
		auto pass = request.uri.parameter("pass");

		const auto& user = Game::UserManager::CreateUserWithNameMailAndPassword(name, mail, pass);

		rapidjson::StringBuffer buffer;
		buffer.Clear();

		rapidjson::Document document;
		document.SetObject();
		if (user == NULL) {
			// stat
			document.AddMember(rapidjson::Value("stat"), rapidjson::Value("error"), document.GetAllocator());
		}
		else {
			// stat
			document.AddMember(rapidjson::Value("stat"), rapidjson::Value("ok"), document.GetAllocator());
		}
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);

		response.set(boost::beast::http::field::content_type, "application/json");
		response.body() = buffer.GetString();
	}

	void API::bootstrap_config_getConfig(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		pugi::xml_document document;

		const auto& host = Config::Get(CONFIG_SERVER_HOST);

		auto docResponse = document.append_child("response");
		if (auto configs = docResponse.append_child("configs")) {
			if (auto config = configs.append_child("config")) {
				utils::xml_add_text_node(config, "blaze_service_name", "darkspore");
				utils::xml_add_text_node(config, "blaze_secure", "Y");
				utils::xml_add_text_node(config, "blaze_env", "production");
				utils::xml_add_text_node(config, "sporenet_cdn_host", host);
				utils::xml_add_text_node(config, "sporenet_cdn_port", "80");
				utils::xml_add_text_node(config, "sporenet_db_host", host);
				utils::xml_add_text_node(config, "sporenet_db_port", "80");
				utils::xml_add_text_node(config, "sporenet_db_name", "darkspore");
				utils::xml_add_text_node(config, "sporenet_host", host);
				utils::xml_add_text_node(config, "sporenet_port", "80");
				utils::xml_add_text_node(config, "http_secure", "N");
				utils::xml_add_text_node(config, "liferay_host", host);
				utils::xml_add_text_node(config, "liferay_port", "80");
				utils::xml_add_text_node(config, "launcher_action", "2");
				utils::xml_add_text_node(config, "launcher_url", "http://" + host + "/bootstrap/launcher/?version=" + mVersion);
			}
		}

		docResponse.append_child("to_image");
		docResponse.append_child("from_image");

		if (request.uri.parameterb("include_settings")) {
			if (auto settings = docResponse.append_child("settings")) {
				utils::xml_add_text_node(settings, "open", "true");
				utils::xml_add_text_node(settings, "telemetry-rate", "256");
				utils::xml_add_text_node(settings, "telemetry-setting", "0");
			}
		}

		if (request.uri.parameterb("include_patches")) {
			docResponse.append_child("patches");
			/*
			target, date, from_version, to_version, id, description, application_instructions,
			locale, shipping, file_url, archive_size, uncompressed_size,
			hashes(attributes, Version, Hash, Size, BlockSize)
			*/
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_status_getStatus(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		if (auto status = docResponse.append_child("status")) {
			utils::xml_add_text_node(status, "health", "1");

			if (auto api = status.append_child("api")) {
				utils::xml_add_text_node(api, "health", "1");
				utils::xml_add_text_node(api, "revision", "1");
				utils::xml_add_text_node(api, "version", "1");
			}

			if (auto blaze = status.append_child("blaze")) {
				utils::xml_add_text_node(blaze, "health", "1");
			}

			if (auto gms = status.append_child("gms")) {
				utils::xml_add_text_node(gms, "health", "1");
			}

			if (auto nucleus = status.append_child("nucleus")) {
				utils::xml_add_text_node(nucleus, "health", "1");
			}

			if (auto game = status.append_child("game")) {
				utils::xml_add_text_node(game, "health", "1");
				utils::xml_add_text_node(game, "countdown", "0");
				utils::xml_add_text_node(game, "open", "1");
				utils::xml_add_text_node(game, "throttle", "0");
				utils::xml_add_text_node(game, "vip", "0");
			}
			/*
			if (auto unk = status.append_child("$\x84")) {
				utils::xml_add_text_node(unk, "health", "1");
				utils::xml_add_text_node(unk, "revision", "1");
				utils::xml_add_text_node(unk, "db_version", "1");
			}

			if (auto unk = status.append_child("$\x8B")) {
				utils::xml_add_text_node(unk, "health", "1");
			}
			*/
		}

		if (request.uri.parameterb("include_broadcasts")) {
			add_broadcasts(docResponse);
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_status_getBroadcastList(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		add_broadcasts(docResponse);
		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_inventory_getPartList(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		if (auto parts = docResponse.append_child("parts")) {
			/*
			if (auto part = parts.append_child("part")) {
				utils::xml_add_text_node(part, "is_flair", "0");
				utils::xml_add_text_node(part, "cost", "1");
				utils::xml_add_text_node(part, "creature_id", "1");
				utils::xml_add_text_node(part, "id", "1");
				utils::xml_add_text_node(part, "level", "1");
				utils::xml_add_text_node(part, "market_status", "0");
				utils::xml_add_text_node(part, "prefix_asset_id", "1");
				utils::xml_add_text_node(part, "prefix_secondary_asset_id", "1");
				utils::xml_add_text_node(part, "rarity", "1");
				utils::xml_add_text_node(part, "reference_id", "1");
				utils::xml_add_text_node(part, "rigblock_asset_id", "0");
				utils::xml_add_text_node(part, "status", "0");
				utils::xml_add_text_node(part, "suffix_asset_id", "0");
				utils::xml_add_text_node(part, "usage", "0");
				utils::xml_add_text_node(part, "creation_date", "0");
			}
			*/
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_inventory_getPartOfferList(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		if (auto docResponse = document.append_child("response")) {
			auto timestamp = utils::get_unix_time();

			utils::xml_add_text_node(docResponse, "expires", timestamp + (3 * 60 * 60 * 1000));
			if (auto parts = docResponse.append_child("parts")) {
				/*
				if (auto part = parts.append_child("part")) {
					utils::xml_add_text_node(part, "is_flair", "0");
					utils::xml_add_text_node(part, "cost", "100");
					utils::xml_add_text_node(part, "creature_id", static_cast<uint64_t>(CreatureID::BlitzAlpha));
					utils::xml_add_text_node(part, "id", "1");
					utils::xml_add_text_node(part, "level", "1");
					utils::xml_add_text_node(part, "market_status", "1");
					utils::xml_add_text_node(part, "prefix_asset_id", 0x010C);
					utils::xml_add_text_node(part, "prefix_secondary_asset_id", "0");
					utils::xml_add_text_node(part, "rarity", "1");
					utils::xml_add_text_node(part, "reference_id", 1);
					utils::xml_add_text_node(part, "rigblock_asset_id", 0x27A1); // 0xA14538E5
					utils::xml_add_text_node(part, "status", "1");
					utils::xml_add_text_node(part, "suffix_asset_id", 0x0005);
					utils::xml_add_text_node(part, "usage", "1");
					utils::xml_add_text_node(part, "creation_date", timestamp);
				}
				*/
			}
			
			add_common_keys(docResponse);
		}

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_account_auth(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		auto key = request.uri.parameter("key");
		if (!key.empty()) {
			// key = auth_token::0
			auto keyData = utils::explode_string(key, "::");

			const auto& user = Game::UserManager::GetUserByAuthToken(keyData.front());
			if (user) {
				session.set_user(user);
			}
		}

		const auto& user = session.get_user();

		/*
			timestamp
				{TIMESTAMP}

			account
				tutorial_completed
				chain_progression
				creature_rewards
				current_game_id
				current_playgroup_id
				default_deck_pve_id
				default_deck_pvp_id
				level
				avatar_id
				id
				new_player_inventory
				new_player_progress
				cashout_bonus_time
				star_level
				unlock_catalysts
				unlock_diagonal_catalysts
				unlock_fuel_tanks
				unlock_inventory
				unlock_pve_decks
				unlock_pvp_decks
				unlock_stats
				unlock_inventory_identify
				unlock_editor_flair_slots
				upsell
				xp
				grant_all_access
				cap_level
				cap_progression

			creatures
				id
				name
				png_thumb_url
				noun_id
				version
				gear_score
				item_points

			decks
				name
				category
				id
				slot
				locked
				creatures
					{SAME_AS_CREATURES_BEFORE}

			feed
				items

			settings

			server_tuning
				itemstore_offer_period
				itemstore_current_expiration
				itemstore_cost_multiplier_basic
				itemstore_cost_multiplier_uncommon
				itemstore_cost_multiplier_rare
				itemstore_cost_multiplier_epic
				itemstore_cost_multiplier_unique
				itemstore_cost_multiplier_rareunique
				itemstore_cost_multiplier_epicunique
		*/

		pugi::xml_document document;
		if (auto docResponse = document.append_child("response")) {
			// Write "account" data
			if (user) {
				auto& account = user->get_account();

				auto newPlayerProgress = request.uri.parameteru("new_player_progress");
				if (newPlayerProgress != std::numeric_limits<uint64_t>::max()) {
					account.newPlayerProgress = newPlayerProgress;
				}

				account.Write(docResponse);
			} else {
				Game::Account account;
				account.Write(docResponse);
			}

			// Other
			pugi::xml_node creatures;
			if (request.uri.parameter("include_creatures") == "true") {
				if (user) {
					user->get_creatures().Write(docResponse);
				} else {
					docResponse.append_child("creatures");
				}
			}

			if (request.uri.parameter("include_decks") == "true") {
				if (user) {
					user->get_squads().Write(docResponse);
				} else {
					docResponse.append_child("decks");
				}
			}

			if (request.uri.parameter("include_feed") == "true") {
				if (user) {
					user->get_feed().Write(docResponse);
				} else {
					docResponse.append_child("feed");
				}
			}

			if (request.uri.parameter("include_server_tuning") == "true") {
				if (auto server_tuning = docResponse.append_child("server_tuning")) {
					auto timestamp = utils::get_unix_time();
					utils::xml_add_text_node(server_tuning, "itemstore_offer_period", timestamp);
					utils::xml_add_text_node(server_tuning, "itemstore_current_expiration", timestamp + (3 * 60 * 60 * 1000));
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_basic", 1);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_uncommon", 1.1);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_rare", 1.2);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_epic", 1.3);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_unique", 1.4);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_rareunique", 1.5);
					utils::xml_add_text_node(server_tuning, "itemstore_cost_multiplier_epicunique", 1.6);
				}
			}

			if (request.uri.parameter("include_settings") == "true") {
				docResponse.append_child("settings");
			}

			if (request.uri.parameter("cookie") == "true") {
				std::string cookie;
				if (user) {
					cookie = user->get_auth_token();
				} else {
					cookie = "TESTING";
				}
				response.set(boost::beast::http::field::set_cookie, "token=" + cookie);
			}

			add_common_keys(docResponse);
		}

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_account_getAccount(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		const auto& user = session.get_user();

		pugi::xml_document document;
		if (auto docResponse = document.append_child("response")) {
			bool include_creatures = request.uri.parameter("include_creatures") == "true";
			bool include_decks = request.uri.parameter("include_decks") == "true";
			bool include_feed = request.uri.parameter("include_feed") == "true";
			bool include_stats = request.uri.parameter("include_stats") == "true";
			if (user) {
				user->get_account().Write(docResponse);

				if (include_creatures) { user->get_creatures().Write(docResponse); }
				if (include_decks) { user->get_squads().Write(docResponse); }
				if (include_feed) { user->get_feed().Write(docResponse); }
				if (include_stats) {
					auto stats = docResponse.append_child("stats");
					auto stat = stats.append_child("stat");
					utils::xml_add_text_node(stat, "wins", 0);
				}
			} else {
				Game::Account account;
				account.Write(docResponse);

				if (include_creatures) { docResponse.append_child("creatures"); }
				if (include_decks) { docResponse.append_child("decks"); }
				if (include_feed) { docResponse.append_child("feed"); }
				if (include_stats) { docResponse.append_child("stats"); }
			}

			add_common_keys(docResponse);
		}

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_account_logout(HTTP::Session& session, HTTP::Response& response) {
		const auto& user = session.get_user();
		if (user) {
			user->Logout();
		}
	}

	void API::game_account_unlock(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		const auto& user = session.get_user();
		if (user) {
			uint32_t unlockId = request.uri.parameteru("unlock_id");
			user->UnlockUpgrade(unlockId);
		}

		game_account_getAccount(session, response);
	}

	void API::game_game_getGame(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		if (auto game = docResponse.append_child("game")) {
			utils::xml_add_text_node(game, "game_id", "1");
			utils::xml_add_text_node(game, "cashed_out", "0");
			utils::xml_add_text_node(game, "finished", "0");
			utils::xml_add_text_node(game, "starting_difficulty", "1");
			utils::xml_add_text_node(game, "start", "1");
			/*
			utils::xml_add_text_node(game, "rounds", "0");
			utils::xml_add_text_node(game, "chain_id", "0");
			utils::xml_add_text_node(game, "finish", "0");
			utils::xml_add_text_node(game, "planet_id", "0");
			utils::xml_add_text_node(game, "success", "0");
			*/
			if (auto players = game.append_child("players")) {
				if (auto player = players.append_child("player")) {
					/*
					utils::xml_add_text_node(player, "deaths", "0");
					utils::xml_add_text_node(player, "kills", "0");
					*/
					utils::xml_add_text_node(player, "account_id", "1");
					utils::xml_add_text_node(player, "result", "0");
					utils::xml_add_text_node(player, "creature1_id", "1");
					utils::xml_add_text_node(player, "creature1_version", "1");
					utils::xml_add_text_node(player, "creature2_id", "0");
					utils::xml_add_text_node(player, "creature2_version", "0");
					utils::xml_add_text_node(player, "creature3_id", "0");
					utils::xml_add_text_node(player, "creature3_version", "0");
				}
			}
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_game_exitGame(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_creature_resetCreature(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		pugi::xml_document document;

		auto docResponse = document.append_child("response");

		const auto& user = session.get_user();
		if (user) {
			Creature* creature = user->GetCreatureById(request.uri.parameteru("id"));
			if (creature) {
				creature->Write(docResponse);
			} else {
				docResponse.append_child("creature");
			}
		} else {
			docResponse.append_child("creature");
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_creature_unlockCreature(HTTP::Session& session, HTTP::Response& response) {
		auto& request = session.get_request();

		uint32_t templateId = request.uri.parameteru("template_id");

		const auto& user = session.get_user();
		if (user) {
			user->UnlockCreature(templateId);
		}

		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		utils::xml_add_text_node(docResponse, "creature_id", templateId);

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::game_creature_getCreature(HTTP::Session& session, HTTP::Response& response) {
		/*
			name_locale_id
			text_locale_id
			name
			type_a

			if not template {
				creator_id
			}

			weapon_min_damage
			weapon_max_damage
			gear_score (default: 0)
			class
			stats_template_ability
				example {
					full_string = item0;item1;itemN
					item = a!b,value
				}

			if not template {
				stats
				stats_template_ability_keyvalues
				stats_ability_keyvalues
			} else {
				stats_template
				stats_template_ability_keyvalues
			}

			if not template {
				parts
					part
			}

			creature_parts
			ability_passive
			ability_basic
			ability_random
			ability_special_1
			ability_special_2
			ability
				id

			if not template {
				png_large_url
				png_thumb_url
			}
		*/
		/*
		auto& request = session.get_request();

		uint32_t templateId = request.uri.parameteru("template_id");

		const auto& user = session.get_user();
		if (user) {
			user->UnlockCreature(templateId);
		}

		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		utils::xml_add_text_node(docResponse, "creature_id", templateId);

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
		*/
	}

	/*
/game/api?version=1&token=cookie
cost = 0
gear = 0.000
id = 749013658
large = iVBORw0KGgoAAAANSUhEUgAAAQAAAAEACAYAAABccqhmAAAgAElEQVR4nOy9Z7Bl13WY+e1wwk0v
large_crc = 692908162
method = api.creature.updateCreature
parts = 117957934
points = 300.000
stats = STR,14,0;DEX,13,0;MIND,23,0;HLTH,100,70;MANA,125,23;PDEF,50,78;EDEF,150,138;CRTR,50,52
stats_ability_keyvalues = 885660025!minDamage,5;885660025!maxDamage,8;885660025!percentToHeal,20;1152331895!duration,20;1152331895!spawnMax,2;424126604!radius,8;424126604!healing,5;424126604!duration,6;424126604!minHealing,21;424126604!maxHealing,32;1577880566!Enrage.damage,9;1577880566!Enrage.duration,30;1577880566!Enrage.healing,35;1829107826!diameter,12;1829107826!damage,6;1829107826!duration,10;1829107826!speedDebuff,75
thumb = iVBORw0KGgoAAAANSUhEUgAAAIAAAACACAYAAADDPmHLAAAgAElEQVR4nGy8adRm11kduJ9zh3f6
thumb_crc = 1921048798
token = ABCDEFGHIJKLMNOPQRSTUVWXYZ
version = 1
	*/

	void API::survey_survey_getSurveyList(HTTP::Session& session, HTTP::Response& response) {
		pugi::xml_document document;

		auto docResponse = document.append_child("response");
		if (auto surveys = docResponse.append_child("surveys")) {
			// Empty
		}

		add_common_keys(docResponse);

		xml_string_writer writer;
		document.save(writer, "\t", 1U, pugi::encoding_latin1);

		response.set(boost::beast::http::field::content_type, "text/xml");
		response.body() = std::move(writer.result);
	}

	void API::add_broadcasts(pugi::xml_node& node) {
		if (auto broadcasts = node.append_child("broadcasts")) {
			/*
			if (auto broadcast = broadcasts.append_child("broadcast")) {
				utils::xml_add_text_node(broadcast, "id", "0");
				utils::xml_add_text_node(broadcast, "start", "0");
				utils::xml_add_text_node(broadcast, "type", "0");
				utils::xml_add_text_node(broadcast, "message", "Hello World!");
				if (auto tokens = broadcast.append_child("tokens")) {
				}
			}
			*/
		}
	}

	void API::add_common_keys(pugi::xml_node& node) {
		utils::xml_add_text_node(node, "stat", "ok");
		utils::xml_add_text_node(node, "version", mVersion);
		utils::xml_add_text_node(node, "timestamp", std::to_string(utils::get_unix_time()));
		utils::xml_add_text_node(node, "exectime", std::to_string(++mPacketId));
	}

	void API::add_common_keys(rapidjson::Document& document) {
		/*
		obj["stat"] = 'ok'
		obj["version"] = self.server.gameVersion
		obj["timestamp"] = self.timestamp()
		obj["exectime"] = self.exectime()
		*/
	}
}
