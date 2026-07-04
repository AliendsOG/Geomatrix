// Ceva game client.cpp : Defines the entry point for the application.
//

#include "Ceva game client.h"

#define NOGDI             
#define NOUSER            
#include "enet/enet.h"
#undef CloseWindow
#undef ShowCursor
#undef PlaySound
#include "raylib.h"

using std::cout;
using std::string;
//network stuff begin
#pragma pack(push, 1)
#define MAX_PLAYERS 10
#define MAX_PROJECTILES 100
enum class PacketType : uint8_t {
	ID_ASSIGNMENT = 0,
	WORLD_STATE = 1,
	MATCH_STATE_CHANGE = 2,
	LOBBY_COUNT = 3,
	FORCE_CONNECT = 4,
	press_play = 5,
	input = 6,
	disconnect=7
};
struct MatchStatePacket {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::MATCH_STATE_CHANGE);
	uint8_t new_state;
	uint8_t winner_id;
};

struct PlayerNetworkState {
	uint16_t pos_x{};
	uint16_t pos_y{};
	uint16_t health{};
	uint8_t id{};
	bool active{};
};
struct projectile_network {
	uint16_t pos_x{};
	uint16_t pos_y{};
	uint16_t radius{};
	uint16_t id{};
};
struct WorldStatePacket {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::WORLD_STATE);
	uint8_t active_player_count{};
	uint8_t projectile_count{};
	uint8_t ammo{};
	PlayerNetworkState players[MAX_PLAYERS];
	projectile_network projectiles[MAX_PROJECTILES];

};
struct force_connect {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::FORCE_CONNECT);
};

struct pressed_play {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::press_play);
};
struct disconnect {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::disconnect);
};
struct player_input {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::input);
	float x{};
	float y{};
	bool attack{};
	float aim_x{};
	float aim_y{};
	player_input(float x, float y, bool a) :
		x(x),
		y(y),
		attack(a)
	{
	}
};
#pragma pack(pop)

//network stuff end
enum class GameState {
	STATE_MENU,
	STATE_lobby,
	STATE_PLAYING,
	STATE_GAME_OVER
};
std::unordered_map<int, string>maps = {
	{0, "maps/default_map.txt"}
};
struct map {
	int height{};
	int width{};
	int scale{};
	int pl_nr{};
	std::vector<std::vector<int>> matrix;
	std::vector<std::vector<int>> pl_pos;
	bool load_file(const std::string& filepath) {
		std::ifstream file(filepath);
		if (!file.is_open()) {
			std::cerr << "Error loading map file: " << filepath << "\n";
			return false;
		}
		file >> height >> width >> scale >> pl_nr;
		matrix.resize(height, std::vector<int>(width, 0));
		pl_pos.clear();

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int tileValue;
				file >> tileValue;
				if (tileValue == -1) {
					int pixelX = x * scale + (scale / 2);
					int pixelY = y * scale + (scale / 2);
					pl_pos.push_back({ pixelY, pixelX });
					matrix[y][x] = 0;
				}
				else {
					matrix[y][x] = tileValue;
				}
			}
		}

		file.close();
		return true;
	}
};
const int pl_width = 96;
struct player {
	int health{};
	string name;
	int pos_x{};
	int pos_y{};
	float pos_f_x{};
	float pos_f_y{};
	int id{};
	bool active = true;
	player(int h, string name, int x, int y,int i) :
		health(h),
		name(name),
		pos_x(x),
		pos_y(y),
		id(i)
	{
	}
};
struct projectile {
	int radius{};
	int pos_x{};
	int pos_y{};
	uint8_t pr_id{};
	float pos_f_x{};
	float pos_f_y{};
};


Color GetCellColor(int value) {
	switch (value) {
	case 0:  return BLACK;        // Ground
	case 1:  return BLUE;		  // Water
	case 2:  return YELLOW;       // Bush
	case 3:  return BROWN;        // wall
	case 4:  return DARKGRAY;     // indestructible Wall
	default: return WHITE;        // Fallback
	}
};
std::vector<Color> pl_colors = {
	Color{ 230, 57,  70,  255 }, 
	Color{ 58,  125, 203, 255 }, 
	Color{ 46,  196, 182, 255 }, 
	Color{ 255, 159, 28,  255 }, 
	Color{ 155, 93,  229, 255 },
	Color{ 0,   204, 102, 255 }, 
	Color{ 255, 0,   127, 255 }, 
	Color{ 241, 91,  181, 255 }, 
	Color{ 255, 214, 10,  255 },
	Color{ 0,   245, 255, 255 }
};
std::vector<Color> pl_colors_team = {
	Color{ 255, 0,  0,  255 },
	Color{ 0,  0, 255, 255 },
};
auto movement_float(bool gamepad) {
	float joy_y = 0, joy_x = 0, aim_x = 0,aim_y=0;
	bool attack = false;
	const float deadzone = 0.1f;
	const float trigger_deadzone = -0.9f;
	if (gamepad) {
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X)) >= deadzone) {
			joy_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y)) >= deadzone) {
			joy_y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X)) >= deadzone) {
			aim_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
		}
		if (abs(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)) >= deadzone) {
			aim_y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
		}
		if (GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER) > trigger_deadzone) {
			attack = true;
		}
	}
	if (IsKeyDown(KEY_UP)) {
		aim_y = -1;
	}
	if (IsKeyDown(KEY_DOWN)) {
		aim_y = 1;
	}
	if (IsKeyDown(KEY_LEFT)) {
		aim_x = -1;
	}
	if (IsKeyDown(KEY_RIGHT)) {
		aim_x = 1;
	}
	if (IsKeyDown(KEY_W)) {
		joy_y = -1;
	}
	if (IsKeyDown(KEY_S)) {
		joy_y = 1;
	}
	if (IsKeyDown(KEY_A)) {
		joy_x = -1;
	}
	if (IsKeyDown(KEY_D)) {
		joy_x = 1;
	}
	if (IsKeyDown(KEY_E)) {
		attack= true;
	}

	
	float lengthSq = joy_x * joy_x + joy_y * joy_y;
	if (lengthSq > 1.0f) {
		float length = std::sqrt(lengthSq);
		joy_x /= length;
		joy_y /= length;
	}
	lengthSq = aim_x * aim_x + aim_y * aim_y;
	if (lengthSq > 1.0f) { 
		float length = std::sqrt(lengthSq);
		aim_x /= length;
		aim_y /= length;
	}

	player_input input(joy_x, joy_y, attack);
	input.aim_x = aim_x;
	input.aim_y = aim_y;
	return input;
}


void SendInputToServer(ENetPeer* peer, player_input input) {
	ENetPacket* packet = enet_packet_create(&input, sizeof(player_input), 0);
	enet_peer_send(peer, 0, packet);
}
void draw_tile(int screenX, int screenY, int size, int cellValue,int x, int y) {
	DrawRectangle(screenX, screenY, size, size, Color{ 15, 18, 26, 255 });

	DrawRectangleLines(screenX, screenY, size, size, Color{ 30, 41, 59, 100 });

	switch (cellValue) {
	case 0: { 
		if ((x * 3 + y * 7) % 5 == 0) {
			DrawLine(screenX + size / 2 - 10, screenY + size / 2, screenX + size / 2 + 10, screenY + size / 2, Color{ 51, 65, 85, 150 });
			DrawLine(screenX + size / 2, screenY + size / 2 - 10, screenX + size / 2, screenY + size / 2 + 10, Color{ 51, 65, 85, 150 });
		}
		break;
	}
	case 1: {
		DrawRectangle(screenX + 4, screenY + 4, size - 8, size - 8, Color{ 25, 30, 45, 255 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 4, (float)screenY + 4, (float)size - 8, (float)size - 8 }, 3.0f, BLUE);
		break;
	}
	case 2: {
		DrawRectangle(screenX + 4, screenY + 4, size - 8, size - 8, Color{ 25, 30, 45, 255 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 4, (float)screenY + 4, (float)size - 8, (float)size - 8 }, 3.0f, YELLOW);
		break;
	}
	case 3: { 
		DrawRectangle(screenX + 4, screenY + 4, size - 8, size - 8, Color{ 25, 30, 45, 255 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 4, (float)screenY + 4, (float)size - 8, (float)size - 8 }, 3.0f, Color{ 255, 120, 0, 255 });
		DrawRectangle(screenX + size / 4, screenY + size / 4, size / 2, size / 2, Color{ 40, 48, 68, 255 });
		break;
	}
	case 4: { 
		DrawRectangle(screenX + 2, screenY + 2, size - 4, size - 4, Color{ 10, 12, 18, 255 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 2, (float)screenY + 2, (float)size - 4, (float)size - 4 }, 4.0f, Color{ 0, 245, 255, 255 });
		DrawLineEx(Vector2{ (float)screenX + 15, (float)screenY + 15 }, Vector2{ (float)screenX + size - 15, (float)screenY + size - 15 }, 2.0f, Color{ 0, 245, 255, 150 });
		DrawLineEx(Vector2{ (float)screenX + size - 15, (float)screenY + 15 }, Vector2{ (float)screenX + 15, (float)screenY + size - 15 }, 2.0f, Color{ 0, 245, 255, 150 });
		break;
	}
	default:
		break;
	}
	
}


void match_drawing( std::optional<WorldStatePacket> (&world_buffer)[3],const std::optional<double>(&time_buffer)[3], std::vector<player>& players, map& map, RenderTexture2D& canvas,const int my_id, const int virtualWidth, const int virtualHeight, const int& max_view_x, const int& max_view_y, const int& windowWidth, const int& windowHeight) {
	float scale = fminf((float)windowWidth / virtualWidth, (float)windowHeight / virtualHeight);

	float targetWidth = virtualWidth * scale;
	float targetHeight = virtualHeight * scale;
	float offsetX = (windowWidth - targetWidth) / 2.0f;
	float offsetY = (windowHeight - targetHeight) / 2.0f;
	BeginTextureMode(canvas);
	ClearBackground(BLACK);
	float current_client_time = GetTime();

	float render_time = current_client_time - 2.0f / 69.0f;
	WorldStatePacket* world_1 = nullptr;
	WorldStatePacket* world_2 = nullptr;
	if (world_buffer[1].has_value()) {
		world_1 = &world_buffer[1].value();
		world_2 = &world_buffer[2].value();
	}

	if (world_1 && world_2 && my_id != -1 && players[my_id].active) {
	DrawText(TextFormat("%d", world_1->active_player_count), 10, 20, 10, GREEN);
	float time_between_packets = time_buffer[2].value() - time_buffer[1].value();
	float elapsed_time = render_time - time_buffer[1].value();
	float t = (time_between_packets > 0.0f) ? (elapsed_time / time_between_packets) : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);
	for (auto& pl : players) {
		pl.active = false;
	}
	for (int i = 0; i < world_1->active_player_count; i++) {
		auto& p1 = world_1->players[i];
		uint8_t p_id = static_cast<uint8_t>(p1.id);
		if (p_id >= players.size()) continue;
		players[p_id].active = true;
		players[p_id].health = p1.health;
		int w2_index = -1;
		for (int j = 0; j < world_2->active_player_count; j++) {
			if (world_2->players[j].id == p_id) {
				w2_index = j;
				break;
			}
		}
		if (w2_index != -1) {
			auto& p2 = world_2->players[w2_index];
			players[p_id].pos_f_x = p1.pos_x + (p2.pos_x - p1.pos_x) * t;
			players[p_id].pos_f_y = p1.pos_y + (p2.pos_y - p1.pos_y) * t;
		}
		else {
			players[p_id].pos_f_x = p1.pos_x;
			players[p_id].pos_f_y = p1.pos_y;
		}
		players[p_id].pos_x = static_cast<int>(players[p_id].pos_f_x);
		players[p_id].pos_y = static_cast<int>(players[p_id].pos_f_y);
	}
	int my_screen_x = players[my_id].pos_x;
	int my_screen_y = players[my_id].pos_y;
	int cam_offset_x = my_screen_x - max_view_x;
	int cam_offset_y = my_screen_y - max_view_y;
	int max_cam_x = (map.width * map.scale) - (2 * max_view_x);
	int max_cam_y = (map.height * map.scale) - (2 * max_view_y);
	cam_offset_x = std::clamp(cam_offset_x, 0, max_cam_x);
	cam_offset_y = std::clamp(cam_offset_y, 0, max_cam_y);
	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			Color tileColor = GetCellColor(map.matrix[y][x]);
			int tile_draw_x = (x * map.scale) - cam_offset_x;
			int tile_draw_y = (y * map.scale) - cam_offset_y;
			if (tile_draw_x >= -map.scale && tile_draw_x <= virtualWidth &&
				tile_draw_y >= -map.scale && tile_draw_y <= virtualHeight) {
				draw_tile(tile_draw_x, tile_draw_y, map.scale, map.matrix[y][x], x, y);
			}
		}
	}

	for (int i = 0; i < world_1->active_player_count; i++) {
		auto& p1 = world_1->players[i];
		uint8_t p_id = static_cast<uint8_t>(p1.id);
		if (p_id >= players.size() || !players[p_id].active) continue;

		int final_draw_x = players[p_id].pos_x - cam_offset_x - (pl_width / 2);
		int final_draw_y = players[p_id].pos_y - cam_offset_y - (pl_width / 2);
		int team_idx = (p_id < players.size() / 2) ? 0 : 1;
		int tile = map.matrix[players[p_id].pos_y / map.scale][players[p_id].pos_x / map.scale];
		if (tile == 2) {
			if (p_id == my_id) {
				pl_colors_team[team_idx].a = 128;
				DrawRectangle(final_draw_x, final_draw_y, pl_width, pl_width, pl_colors_team[team_idx]);
				DrawText(TextFormat("%d", players[p_id].health), final_draw_x, final_draw_y - 96, 64, GREEN);
				DrawText(TextFormat("%d", world_1->ammo), final_draw_x, final_draw_y - 160, 64, RED);
			}
		}
		else {
			pl_colors_team[team_idx].a = 255;
			DrawRectangle(final_draw_x, final_draw_y, pl_width, pl_width, pl_colors_team[team_idx]);
			DrawText(TextFormat("%d", players[p_id].health), final_draw_x, final_draw_y - 96, 64, GREEN);
			if (p_id == my_id) {
				DrawText(TextFormat("%d", world_1->ammo), final_draw_x, final_draw_y - 160, 64, RED);
			}
		}
	}

	for (int i = 0; i < world_1->projectile_count; i++) {
		auto& proj1 = world_1->projectiles[i];
		uint8_t target_p_id = proj1.id;
		int w2_index = -1;
		for (int j = 0; j < world_2->projectile_count; j++) {
			if (world_2->projectiles[j].id == target_p_id) {
				w2_index = j;
				break;
			}
		}
		float proj_screen_f_x = 0.0f;
		float proj_screen_f_y = 0.0f;
		if (w2_index != -1) {
			auto& proj2 = world_2->projectiles[w2_index];
			proj_screen_f_x = proj1.pos_x + t * (proj2.pos_x - proj1.pos_x);
			proj_screen_f_y = proj1.pos_y + t * (proj2.pos_y - proj1.pos_y);
		}
		else {
			proj_screen_f_x = proj1.pos_x;
			proj_screen_f_y = proj1.pos_y;
		}
		int proj_screen_x = static_cast<int>(proj_screen_f_x);
		int proj_screen_y = static_cast<int>(proj_screen_f_y);
		int final_proj_x = proj_screen_x - cam_offset_x - (proj1.radius / 2);
		int final_proj_y = proj_screen_y - cam_offset_y - (proj1.radius / 2);
		DrawCircle(final_proj_x, final_proj_y, proj1.radius, Color{ 155, 93, 229, 255 });
	}
}
			else {
	DrawText("CONNECTING TO SERVER...", virtualWidth / 2 - 140, virtualHeight / 2 - 10, 20, RAYWHITE);
	}
	EndTextureMode();
	BeginDrawing();
	ClearBackground(BLACK);
	Rectangle sourceRec = { 0.0f, 0.0f, (float)virtualWidth, -(float)virtualHeight };
	Rectangle destRec = { offsetX, offsetY, targetWidth, targetHeight };
	Vector2 origin = { 0.0f, 0.0f };
	DrawTexturePro(canvas.texture, sourceRec, destRec, origin, 0.0f, WHITE);
	DrawFPS(10, 10);
	EndDrawing();
}
struct menu_option {
	string name;
	bool selected = false;
};
class button {
public:
	Rectangle bounds;
	Color baseColor;
	Color hoverColor;
	Color activeColor;
	const char* text;
	int fontSize;
	button(float x, float y, float width, float height, const char* buttonText, int size) {
		bounds = { x, y, width, height };
		baseColor = RED;
		hoverColor = YELLOW;
		activeColor = BLACK;
		text = buttonText;
		fontSize = size;
	}
	bool IsPressed(Vector2 mousePos) {
		return CheckCollisionPointRec(mousePos, bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	}
	void Draw(Vector2 mousePos, bool selected) {
		Color currentColor = baseColor;
		if (CheckCollisionPointRec(mousePos, bounds)) {
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				currentColor = activeColor;
			}
			else {
				currentColor = hoverColor;
			}
		}
		if (selected) {
			currentColor = hoverColor;
		}
		DrawRectangleRounded(bounds, 0.69, 9, currentColor);

		int textWidth = MeasureText(text, fontSize);
		float textX = bounds.x + (bounds.width - textWidth) / 2;
		float textY = bounds.y + (bounds.height - fontSize) / 2;
		DrawText(text, textX, textY, fontSize, WHITE);
	}

};

int main() {
	bool active = true;
	int my_id=-1;
	std::vector<player> players;
	std::vector<projectile> projectiles;
	std::optional<WorldStatePacket> world_buffer[3]{};
	std::optional<double> time_buffer[3]{};
	menu_option menu[4];
	string menu_names[4] = {"Play","Change Shape", "Change Map", "Settings"};
	for (int i = 0; i < 4; i++) menu[i].name = menu_names[i];
	menu[0].selected = true;
	map current_map;
	auto file_name = maps.find(0);
	if (file_name != maps.end()) {
		if (!current_map.load_file(file_name->second)) {
			cout << "No map found in file" << std::endl;
			std::cin.get();
			return -1;
		}
		else {
			cout << "map loaded succesfully, width " << current_map.width << " height " << current_map.height << std::endl;
		}
	}
	int max_view_x = current_map.width * current_map.scale / 2;
	int max_view_y = current_map.width * current_map.scale / 2 / 16 * 9;
	GameState current_state = GameState::STATE_MENU;

	for (int i = 0; i < current_map.pl_pos.size(); i++) {
		player pl_temp(1000, "Player" + std::to_string(i), current_map.pl_pos[i][1], current_map.pl_pos[i][0],i);
		pl_temp.active = true;
		players.push_back(pl_temp);
	}
	if (enet_initialize() != 0) {
		std::cerr << "An error occurred while initializing ENet.\n";
		return 1;
	}
	atexit(enet_deinitialize);
	ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0); 
	if (client == nullptr) {
		std::cerr << "An error occurred while trying to create an ENet client host.\n";
		return 1;
	}

	ENetAddress address;
	enet_address_set_host(&address, "127.0.0.1"); 
	address.port = 12345;                        

	ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
	if (peer == nullptr) {
		std::cerr << "No available peers for initiating an ENet connection.\n";
		return 1;
	}

	ENetEvent event;
	if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		std::cout << "Connection to localhost:12345 succeeded!\n";
	}
	else {
		enet_peer_reset(peer);
		std::cerr << "Connection to localhost:12345 failed.\n";
		enet_host_destroy(client);
		return 1;
	}



	const int virtualWidth = 2 * max_view_x;
	const int virtualHeight = 2 * max_view_y;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	int win_width = 1280;
	int win_height = 720;
	InitWindow(win_width, win_height, "ceva game client");
	SetTargetFPS(144);
	RenderTexture2D canvas = LoadRenderTexture(virtualWidth, virtualHeight);
	SetTextureFilter(canvas.texture, TEXTURE_FILTER_TRILINEAR);
	int players_waiting=0;
	int my_result = 0;
	while (active && !WindowShouldClose()) {
		if (IsKeyDown(KEY_X)|| WindowShouldClose()) {
			active = false;
			disconnect ceva;
			ENetPacket* packet= enet_packet_create(&ceva, sizeof(disconnect), ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet);
		}
		int windowWidth = GetScreenWidth();
		int windowHeight = GetScreenHeight();
		while (enet_host_service(client, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_RECEIVE: {
				uint8_t packet_type = event.packet->data[0];
				if (packet_type == static_cast<uint8_t>(PacketType::ID_ASSIGNMENT)) {
					 
					my_id = event.packet->data[1];
					cout << "Server assigned me Player ID: " << my_id << std::flush;
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::WORLD_STATE)) {
					if (event.packet->dataLength < 3) {
						enet_packet_destroy(event.packet);
						break;
					}

					WorldStatePacket incoming_world{};
					uint8_t server_players = event.packet->data[1];
					uint8_t server_projectiles = event.packet->data[2];
					uint8_t ammo = event.packet->data[3];
					size_t expected_bytes = 4
						+ (server_players * sizeof(PlayerNetworkState))
						+ (server_projectiles * sizeof(projectile_network));
					if (event.packet->dataLength < expected_bytes) {
						std::cout << "Warning: Truncated packet dropped! Expected "
							<< expected_bytes << ", got " << event.packet->dataLength << "\n";
						enet_packet_destroy(event.packet);
						break;
					}
					incoming_world.packet_type = event.packet->data[0];
					incoming_world.active_player_count = server_players;
					incoming_world.projectile_count = server_projectiles;
					incoming_world.ammo = ammo;
					uint8_t* payload_src = event.packet->data + 4;
					for (int i = 0; i < incoming_world.active_player_count && i < MAX_PLAYERS; i++) {
						std::memcpy(&incoming_world.players[i], payload_src, sizeof(PlayerNetworkState));
						payload_src += sizeof(PlayerNetworkState);
						uint8_t p_id = incoming_world.players[i].id;
						if (p_id < players.size()) {
							players[p_id].id = p_id;
							players[p_id].health = incoming_world.players[i].health;
							players[p_id].active = incoming_world.players[i].active;
							if (p_id == my_id) {
								players[p_id].pos_x = incoming_world.players[i].pos_x;
								players[p_id].pos_y = incoming_world.players[i].pos_y;
							}

						}
					}
					for (int i = 0; i < incoming_world.projectile_count && i < MAX_PROJECTILES; i++) {
						std::memcpy(&incoming_world.projectiles[i], payload_src, sizeof(projectile_network));
						payload_src += sizeof(projectile_network);
					}

					world_buffer[1] = world_buffer[2];
					time_buffer[1] = time_buffer[2];
					world_buffer[2] = incoming_world;
					time_buffer[2] = GetTime();
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::MATCH_STATE_CHANGE)) {
					MatchStatePacket* state_pkt = reinterpret_cast<MatchStatePacket*>(event.packet->data);
					if (state_pkt->new_state == 1) {
						current_state = GameState::STATE_PLAYING;
					}
					else if (state_pkt->new_state == 2) { 
						current_state = GameState::STATE_GAME_OVER;
						my_result = event.packet->data[2];
					}
					else if (state_pkt->new_state == 0) {
						current_state = GameState::STATE_MENU;
					}
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::LOBBY_COUNT)) {
					players_waiting = event.packet->data[1];
				}
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: {
				std::cout << "The server disconnected or kicked you." << std::endl;
				break;
			}

			default:
				break;
			}
		}
		if (!active)continue;
		switch (current_state) {
		case GameState::STATE_MENU: {
			BeginDrawing();
			ClearBackground(DARKBLUE);
			int text_width = MeasureText("Geomatrix", 50);
			bool selected = false;
			int option = -1;
			DrawText("Geomatrix", windowWidth / 2 - text_width/2, windowHeight / 3, 50, YELLOW);
			for (int i = 0; i < 4; i++) {
				button button(windowWidth / 2-125, windowHeight / 2 + 30 * i,250,25, menu[i].name.c_str(), 20);
				Vector2 mousePos = GetMousePosition();
				button.Draw(mousePos, menu[i].selected);
				if (button.IsPressed(mousePos)) {
					selected = true;
					option = i;
				}
			}
			if (IsKeyPressed(KEY_DOWN)) {
				for (int i = 0; i < 4; i++) {
					if (menu[i].selected) {
						menu[i].selected = false;
						if (i+1 < 4) {
							menu[i+1].selected = true;
						}
						else {
							menu[0].selected = true;
						}
						break;
					}
				}
			}
			if (IsKeyPressed(KEY_UP)) {
				for (int i = 0; i < 4; i++) {
					if (menu[i].selected) {
						menu[i].selected = false;
						if (i-1 >=0) {
							menu[i-1].selected = true;
						}
						else {
							menu[3].selected = true;
						}
						break;
					}
				}
			}

			EndDrawing();
			if (IsKeyPressed(KEY_ENTER)) {
				for (int i = 0; i < 4; i++) {
					if (menu[i].selected) {
						option = i;
						selected = true;
					}
				}
			}
			if (selected) {
				switch (option) {
				case 0: {
					current_state = GameState::STATE_lobby;
					pressed_play play;
					ENetPacket* packet = enet_packet_create(&play, sizeof(pressed_play), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					break;
				}
				case 1: {
					break;
				}
				case 2: {
					break;
				}
				case 3: {
					break;
				}
				default:
					break;
				}
			}

			break;
		}
		case GameState::STATE_lobby: {
			BeginDrawing();
			ClearBackground(DARKBLUE);
			if (IsKeyDown(KEY_B)) {
				force_connect ceva;
				ENetPacket* packet = enet_packet_create(&ceva, sizeof(force_connect), ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
			}
			DrawText("Waiting for players", windowWidth / 2 - 180, windowHeight / 3, 30, YELLOW);
			DrawText(TextFormat("%d / %d", players_waiting, current_map.pl_nr), windowWidth / 2 - 150, windowHeight / 2, 20, RAYWHITE);
			EndDrawing();
			break;
		}
		case GameState::STATE_PLAYING: {
			player_input input = movement_float(IsGamepadAvailable(0));
			if (my_id != -1) {
				SendInputToServer(peer, input);
			}
				match_drawing(world_buffer, time_buffer, players, current_map, canvas, my_id, virtualWidth, virtualHeight, max_view_x, max_view_y,windowWidth,windowHeight);
			break;
		}
		case GameState::STATE_GAME_OVER: {
			while (enet_host_service(client, &event, 0) > 0) { 
				enet_packet_destroy(event.packet); 
			}
			for (int i = 0; i < current_map.pl_pos.size(); i++) {
				player pl_temp(1000, "Player" + std::to_string(i), current_map.pl_pos[i][1], current_map.pl_pos[i][0], i);
				pl_temp.active = true;
				players.push_back(pl_temp);
			}
			projectiles.clear();
			BeginDrawing();
			ClearBackground(MAROON);
			DrawText("GAME OVER!", windowWidth / 2 - 160, windowHeight / 3, 30, WHITE);
			DrawText(TextFormat("Your rank: %d", my_result), windowWidth / 2 - 160, windowHeight / 2.5, 30, WHITE);
			DrawText("PRESS [ENTER] TO RETURN TO MENU", windowWidth / 2 - 160, windowHeight / 2, 18, GRAY);
			EndDrawing();
			if (IsKeyPressed(KEY_ENTER)) {
				current_state = GameState::STATE_MENU;
			}
			break;
		}
		}
	}
	UnloadRenderTexture(canvas);
	CloseWindow();
	return 0;
}
