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
	disconnect=7,
	select_map = 8,
	select_shape = 9
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
	uint8_t shape_id{};
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
	uint8_t shape_id{};
	uint8_t shape_level{};
};
struct disconnect {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::disconnect);
};
struct map_select {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::select_map);
	uint8_t map_id{};
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
	STATE_GAME_OVER,
	state_map_select,
	state_shape_select,
	state_settings_menu,
	state_shape_screen,
};
const std::unordered_map<int, string>maps = {
	{0, "maps/default_map.txt"},
	{1, "maps/ceva.txt"}
};
const std::map<string, int>maps_o = {
	{"Default",0},
	{"Ceva",1}
};
const std::unordered_map<int, string>maps_id_name = {
	{0,"Default"},
	{1,"Ceva"}
};
struct map {
	int height{};
	int width{};
	int scale{};
	int pl_nr{};
	int id{};
	std::vector<std::vector<int>> matrix;
	std::vector<std::vector<int>> pl_pos;
	bool load_file(const string& filepath) {
		std::ifstream file(filepath);
		if (!file.is_open()) {
			std::cerr << "Error loading map file: " << filepath << "\n";
			return false;
		}
		file >> height >> width >> scale >> pl_nr;
		matrix.clear();
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
		if (pl_nr != (int)pl_pos.size()) {
			std::cout << "Info: Corrected pl_nr from " << pl_nr
				<< " to actual spawn nodes found: " << pl_pos.size() << "\n";
			pl_nr = (int)pl_pos.size();
		}

		file.close();
		return true;
	}
};
struct level_stats {
	int health{};
	int ammo_damage{};
};
struct shape_ch {
	int id{};
	string name{};
	int level{};
	int health{};
	std::array<level_stats, 3> levels;
};
const int level_up_prices[2] = {256, 512 };
shape_ch shapes[3] = {
	{
		.id = 0,
		.name = "Square",
		.level = 0,
		.levels = {{
			{1000, 100}, 
			{1200, 120}, 
			{1450, 145} 
		}}
	},
	{
		.id = 1,
		.name = "Circle",
		.level = 0,
		.levels = {{
			{1500,  65 },
			{1700,  80 },
			{1950,  105 },
		}}
	},
	{
		.id = 2,
		.name = "Superellipse",
		.level = 0,
		.levels = {{
			{1250,  150 },
			{1450,  170 },
			{1800,  200 },
		}}
	}
};
const int shapes_price_reals_array[std::size(shapes)] = {0, 256, 512};
const int shapes_price_imaginaries_array[std::size(shapes)] = { 0, 4, 8};
const std::unordered_map<string, int>shapes_map = {
	{"Square",0},
	{"Circle",1},
	{"Superellipse",2}
};
std::vector<shape_ch>shapes_unlocked={
	{
		.id = 0,
		.name = "Square",
		.level = 0,
		.levels = {{
			{1000,  100 },
			{1200,  120 },
			{1450,  145 },
		}}
	}
};
const string settings_array[4] = {
	{"Resolution"},
	{"Fullscreen"},
	{"Target FPS"},
	{"Scale"},
};
const int resolutions_matrix[7][7] = {
	{1280,720},
	{ 1600,900 },
	{1920,1080},
	{2560,1440},
	{3200,1800},
	{3840,2160},
	{7680,4320}

};
const int FPS_array[9] = {
	360,
	240,
	165,
	144,
	120,
	90,
	75,
	60,
	30
};
const float scale_array[6] = {
	1.0,
	0.9,
	0.8,
	0.7,
	0.6,
	0.5
};
int pl_width = 96;
struct player {
	int health{};
	shape_ch shape;
	string name;
	int pos_x{};
	int pos_y{};
	float pos_f_x{};
	float pos_f_y{};
	int id{};
	bool active = true;
	float visible_delay{};
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
class settings {
	public:
		int targetFPS = 144;
		bool fullscreen{};
		bool tutorial = true;
		int resolution_width= 1280;
		int resolution_height = 720 ;
		int last_map_id = 0;
		int last_shape_id = 0;
		int reals = 0;
		int imaginaries = 0;
		int quanta = 0;
		float scale = 0.5;
		string filename = "settings.txt";
		string shapes_file = "shapes.txt";
		void load() {
			std::ifstream infile(filename);

			if (!infile.is_open()) {
				cout << "Settings file doesn't exist. Creating defaults...\n";
				save();
				return;
			}

			string line;
			while (std::getline(infile, line)) {
				std::istringstream iss(line);                
				string key;                
				if (std::getline(iss, key, '=')) {
					string value; 
					if (std::getline(iss, value)) {

						if (key == "TargetFPS")    targetFPS = std::stoi(value);
						else if (key == "Fullscreen") fullscreen = (value == "1" || value == "true");
						else if (key == "Tutorial") tutorial = (value == "1" || value == "true");
						else if (key == "Res Height") resolution_height = std::stoi(value);
						else if (key == "Res Width") resolution_width = std::stoi(value);
						else if (key == "Last Map Id") last_map_id = std::stoi(value);
						else if (key == "Last Shape Id") last_shape_id = std::stoi(value);
						else if (key == "Reals") reals = std::stoi(value);
						else if (key == "Quanta") quanta = std::stoi(value);
						else if (key == "Scale") scale = std::stof(value);
					}
				}
			}
			infile.close();
		}
		void load_shapes() {
			std::ifstream infile(shapes_file);
			if (!infile.is_open()) {
				cout << "Settings file doesn't exist. Creating defaults...\n";
				save_shape();
				return;
			}
			string line;
			while (std::getline(infile, line)) {
				std::istringstream iss(line);
				string key;
				if (std::getline(iss, key, ' ')) {
					string value;
					if (std::getline(iss, value)) {
						if (std::stoi(key) == 0) {
							shapes_unlocked[std::stoi(key)].level = std::stoi(value);
						}
						else {
							shape_ch shape;
							shape.id = std::stoi(key);
							shape.level = std::stoi(key);
							shape.name = shapes[shape.id].name;
							shape.levels = shapes[shape.id].levels;
							shapes_unlocked.push_back(shape);
						}
					}
				}
			}

			infile.close();

		}
		void save_shape() {
			std::ofstream outfile(shapes_file, std::ios::out | std::ios::trunc);
			if (outfile.is_open()) {
				for (int i = 0; i < shapes_unlocked.size(); i++) {
					outfile << shapes_unlocked[i].id << ' ' << shapes[i].level<<"\n";
				}
				outfile.close();
			}
		}
		void save() {
			std::ofstream outfile(filename, std::ios::out | std::ios::trunc);

			if (outfile.is_open()) {
				outfile << "TargetFPS=" << targetFPS << "\n";
				outfile << "Fullscreen=" << fullscreen<< "\n";
				outfile << "Tutorial=" << tutorial << "\n";
				outfile << "Res Height=" << resolution_height << "\n"; 
				outfile << "Res Width=" << resolution_width<< "\n";
				outfile << "Last Map Id=" << last_map_id << "\n";
				outfile << "Last Shape Id=" << last_shape_id << "\n";
				outfile << "Quanta=" << quanta << "\n";
				outfile << "Reals=" << reals << "\n";
				outfile << "Scale=" << scale << "\n";
				outfile.close();
			}
		}
};

void SendInputToServer(ENetPeer* peer, player_input input) {
	ENetPacket* packet = enet_packet_create(&input, sizeof(player_input), 0);
	enet_peer_send(peer, 0, packet);
}
void DrawSuperellipse(int centerX, int centerY, float radiusX, float radiusY, float n, Color color, int segments = 1024) {
	std::vector<Vector2> points(segments + 1);

	for (int i = 0; i <= segments; i++) {
		float angle = (i * 2.0f * PI) / segments;

		float t_cos = cosf(angle);
		float t_sin = sinf(angle);

		float x_mag = powf(fabsf(t_cos), 2.0f / n) * radiusX;
		float y_mag = powf(fabsf(t_sin), 2.0f / n) * radiusY;

		float sign_x = (t_cos >= 0.0f) ? 1.0f : -1.0f;
		float sign_y = (t_sin >= 0.0f) ? 1.0f : -1.0f;

		points[i] = {
			centerX + (x_mag * sign_x),
			centerY + (y_mag * sign_y)
		};
	}

	for (int i = 0; i < segments; i++) {
		Vector2 center = { (float)centerX, (float)centerY };
		DrawTriangle(center, points[i + 1], points[i], color);
	}
}
class water_tile {
private:
	float amplitude;
	float frequency;
	float speed;
	float thickness;
	Color color;
public:
	water_tile(float amp, float freq, float spd, float thick, Color col)
		: amplitude(amp), frequency(freq), speed(spd), thickness(thick), color(col) {
	}

	void Draw(float tileOriginX, float pos_y, int width, float time, float scale_factor) {
		std::vector<Vector2> points;

		float left_padding = 5.0f * scale_factor;

		for (int x = 0; x < width; x += 2) {
			float absolute_screen_x = tileOriginX + left_padding + x;

			float universal_x = absolute_screen_x / scale_factor;

			float current_phase = (universal_x * frequency) + (time * speed);
			float y = width / 2.0f + std::sin(current_phase) * amplitude;

			points.push_back({ tileOriginX + left_padding + x, y + pos_y });
		}

		for (size_t i = 0; i < points.size() - 1; i++) {
			DrawLineEx(points[i], points[i + 1], thickness, color);
		}
	}
};
void draw_tile(int screenX, int screenY, int size, int cellValue,int x, int y ) {
	DrawRectangle(screenX, screenY, size, size, Color{ 15, 18, 26, 255 });


	DrawRectangleLinesEx(Rectangle{ (float)screenX , (float)screenY , (float)size , (float)size }, 5.0f*size/128.0, Color{ 30, 41, 59, 100 });
	switch (cellValue) {
	case 0: { 
		if ((x * 3 + y * 7) % 5 == 0) {

			DrawLineEx(Vector2{ (float)screenX + std::floor(size / 2 - 15 * size / 128.0f) ,(float)screenY + size / 2 }, Vector2{ (float)screenX + size / 2 + std::floor(15 * size / 128.0f ),(float)screenY + size / 2 }, 5.0f*size/128.0f, Color{ 51, 65, 85, 200 });
			DrawLineEx(Vector2{ (float)screenX + size / 2  ,(float)screenY + size / 2 - std::floor(15 * size / 128.0f) }, Vector2{ (float)screenX + size / 2  ,(float)screenY + size / 2 + std::floor(15 * size / 128.0f) }, 5.0f * size / 128.0f, Color{ 51, 65, 85, 200 });
		}
		break;
	}
	case 1: {
		float scale_factor = size / 128.0f; 

		DrawRectangle(screenX + 4 * scale_factor, screenY + 4 * scale_factor, size - 8 * scale_factor, size - 8 * scale_factor, Color{ 25, 30, 45, 255 });
		Color water_col = Color{ 0, 121, 250, 200 };

		water_tile water(35.0f * scale_factor, 0.069f, 2.69f, 5.0f * scale_factor, water_col);
		float time = GetTime();


		water.Draw(
			(float)screenX,
			(float)screenY + 4 * scale_factor,
			size - 8 * scale_factor,
			time,
			scale_factor
		);

		DrawRectangleLinesEx(Rectangle{ (float)screenX + 4 * scale_factor, (float)screenY + 4 * scale_factor, (float)size - 8 * scale_factor, (float)size - 8 * scale_factor }, 5.0f * scale_factor, water_col);

		break;
	}
	case 2: {
		float time = GetTime();

		float noise = sinf(x * 0.25f + time * 1.5f) * cosf(y * 0.3f - time * 1.1f) +
			sinf(y * 0.15f + time * 0.7f) * cosf(x * 0.2f + time * 1.8f);

		noise /= 1.4f;

		unsigned char green_mid = (unsigned char)(90 + (35 * noise)); 
		Color leaf_mid_color = { green_mid+50, green_mid, 0, 255 };
		DrawRectangle(screenX + 4 , screenY + 4 , size - 8, size - 8, leaf_mid_color);


		break;
	}
	case 3: { 
		DrawRectangle(screenX + 4 * size / 128.0f, screenY + 4 * size / 128.0f, size - 8 * size / 128.0f, size - 8 * size / 128.0f, Color{ 25, 30, 45, 255 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 4, (float)screenY + 4, (float)size - 8, (float)size - 8 }, 5.0f * size / 128.0f, Color{ 255, 120, 0, 200 });
		DrawRectangle(screenX + size / 4, screenY + size / 4, size / 2 , size / 2 , Color{ 40, 48, 68, 200 });
		break;
	}
	case 4: { 
		DrawRectangle(screenX + 2, screenY + 2, size - 4, size - 4, Color{ 10, 12, 18, 200 });
		DrawRectangleLinesEx(Rectangle{ (float)screenX + 2, (float)screenY + 2, (float)size - 4, (float)size - 4 }, 5.0f * size / 128.0f, Color{ 0, 245, 255, 200 });
		DrawLineEx(Vector2{ (float)screenX + 15 * size / 128.0f, (float)screenY + 15 * size / 128.0f }, Vector2{ (float)screenX + size - 15 * size / 128.0f, (float)screenY + size - 15 * size / 128.0f }, 4.0f * size / 128.0f, Color{ 0, 245, 255, 150 });
		DrawLineEx(Vector2{ (float)screenX + size - 15 * size / 128.0f, (float)screenY + 15 * size / 128.0f }, Vector2{ (float)screenX + 15 * size / 128.0f, (float)screenY + size - 15 * size / 128.0f }, 4.0f * size / 128.0f, Color{ 0, 245, 255, 150 });
		break;
	}
	default:
		break;
	}
	
}
void draw_player_shape(int shape_id, int center_x, int center_y, Color col) {
	switch (shape_id){
	case 0: {
		DrawRectangle(center_x - pl_width / 2, center_y - pl_width / 2, pl_width, pl_width, col);
		break;
	}
	case 1: {
		DrawCircle(center_x, center_y, pl_width  / 2, col);
		break; 
	}
	case 2: {
		DrawSuperellipse(center_x, center_y, pl_width / 2, pl_width / 2, 4.0f, col);
		break;
	}
	default:
		break;
	}
}

void match_drawing( 
	std::optional<WorldStatePacket> (&world_buffer)[3],
	const std::optional<double>(&time_buffer)[3], 
	std::vector<player>& players, 
	map& map,
	RenderTexture2D& canvas,
	const int my_id, 
	const int virtualWidth,
	const int virtualHeight, 
	const int& max_view_x, 
	const int& max_view_y, 
	const int& windowWidth,
	const int& windowHeight,
	float scale_r) {

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
				int tile_draw_x = ((x * map.scale ) - cam_offset_x) * scale_r;
				int tile_draw_y = ((y * map.scale ) - cam_offset_y) * scale_r;
				if (tile_draw_x >= -map.scale * scale_r && tile_draw_x <= virtualWidth &&
					tile_draw_y >= -map.scale * scale_r && tile_draw_y <= virtualHeight) {
					draw_tile(tile_draw_x, tile_draw_y , map.scale * scale_r, map.matrix[y][x], x, y);
				}
			}
		}

		for (int i = 0; i < world_1->active_player_count; i++) {
			auto& p1 = world_1->players[i];
			uint8_t p_id = static_cast<uint8_t>(p1.id);
			if (p_id >= players.size() || !players[p_id].active) continue;
			int final_draw_x = players[p_id].pos_x - cam_offset_x ;
			int final_draw_y = players[p_id].pos_y - cam_offset_y ;
			int team_idx = p_id;
			int tile = map.matrix[players[p_id].pos_y / map.scale][players[p_id].pos_x / map.scale];
			if (tile == 2) {
				if (p_id == my_id) {
					pl_colors[team_idx].a = 169;
					draw_player_shape(players[p_id].shape.id, final_draw_x * scale_r, final_draw_y * scale_r, pl_colors[team_idx]);
					DrawText(TextFormat("%d", players[p_id].health), players[p_id].pos_x * scale_r -MeasureText(TextFormat("%d", players[p_id].health), 64 * scale_r)/2, final_draw_y * scale_r - 80*scale_r-pl_width/2, 64 * scale_r, GREEN);
					DrawText(TextFormat("%d", world_1->ammo), players[p_id].pos_x * scale_r - MeasureText(TextFormat("%d", world_1->ammo), 64 * scale_r) / 2, (final_draw_y - 144) * scale_r - pl_width / 2, 64 * scale_r, RED);
				}
				else {
					int distance_x = abs(players[p_id].pos_x - players[my_id].pos_x);
					int distance_y = abs(players[p_id].pos_y - players[my_id].pos_y);
					if ((distance_x <= map.scale * 3)&& (distance_y <= map.scale * 3)) {
						pl_colors[team_idx].a = 169;
						draw_player_shape(players[p_id].shape.id, final_draw_x * scale_r, final_draw_y * scale_r, pl_colors[team_idx]);
						DrawText(TextFormat("%d", players[p_id].health), players[p_id].pos_x * scale_r - MeasureText(TextFormat("%d", players[p_id].health), 64 * scale_r) / 2, final_draw_y * scale_r - 80 * scale_r - pl_width / 2, 64 * scale_r, GREEN);
					}
					if (world_1->players[p_id].health!= world_2->players[p_id].health) {
						players[p_id].visible_delay = 0.369;
					}
					if (players[p_id].visible_delay>0.0) {
						players[p_id].visible_delay -= 2.0 / 69.0;
						pl_colors_team[team_idx].a = 169;
						draw_player_shape(players[i].shape.id, final_draw_x * scale_r, final_draw_y * scale_r, pl_colors[team_idx]);
						DrawText(TextFormat("%d", players[p_id].health), players[p_id].pos_x * scale_r - MeasureText(TextFormat("%d", players[p_id].health), 64 * scale_r) / 2, final_draw_y * scale_r - 80 * scale_r - pl_width / 2, 64 * scale_r, GREEN);
					}
					else {
						players[p_id].visible_delay = 0.0;
					}
				}
			}
			else {
				pl_colors[team_idx].a = 255;
				draw_player_shape(players[p_id].shape.id, final_draw_x * scale_r, final_draw_y * scale_r, pl_colors[team_idx]);
				DrawText(TextFormat("%d", players[p_id].health), players[p_id].pos_x * scale_r - MeasureText(TextFormat("%d", players[p_id].health), 64 * scale_r) / 2, final_draw_y * scale_r - 80 * scale_r - pl_width / 2, 64 * scale_r, GREEN);
				if (p_id == my_id) {
					DrawText(TextFormat("%d", world_1->ammo), players[p_id].pos_x * scale_r - MeasureText(TextFormat("%d", world_1->ammo), 64 * scale_r) / 2, final_draw_y * scale_r - 144 * scale_r - pl_width / 2, 64 * scale_r, RED);
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
			int final_proj_x = (proj_screen_x - cam_offset_x) * scale_r;
			int final_proj_y = (proj_screen_y - cam_offset_y) * scale_r;
			DrawCircle(final_proj_x, final_proj_y, proj1.radius* scale_r, Color{ 155, 93, 229, 255 });
		}
	}
	else {
	DrawText("CONNECTING TO SERVER...", virtualWidth / 2 - 140, virtualHeight / 2 - 10, 200 * scale_r, RAYWHITE);
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
	int data{};
	std::vector<menu_option> sub_options;
};
class button {
public:
	Rectangle bounds;
	Color baseColor;
	Color hoverColor;
	Color activeColor;
	Color text_color;
	float corner_r;
	const char* text;
	int fontSize;
	button(float x, float y, float width, float height, const char* buttonText, int size) {
		bounds = { x, y, width, height };
		baseColor = RED;
		hoverColor = YELLOW;
		activeColor = BLACK;
		text = buttonText;
		fontSize = size;
		corner_r = 0.69;
		text_color = WHITE;
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
		DrawRectangleRounded(bounds, corner_r, 255, currentColor);

		int textWidth = MeasureText(text, fontSize);
		float textX = bounds.x + (bounds.width - textWidth) / 2;
		float textY = bounds.y + (bounds.height - fontSize) / 2;
		DrawText(text, textX, textY, fontSize, text_color);
	}

};
class menu_std {
private:
	bool selected = false;
	bool details = false;
	bool selected_sm = false;
	int sent_id_sm = -1;
	int sent_id = -1;
	float delay = 0.269;
	int main_menu_index = 0;
	int submenu_index = 0;
	std::vector<menu_option> map_menu;

public:
	void add_option_main(string name, int index) {
		menu_option menu_op_temp;
		menu_op_temp.name = name;
		menu_op_temp.data = index;
		map_menu.push_back(menu_op_temp);
		if (map_menu.size() == 1) map_menu[0].selected = true;
		
	}
	void add_option_sub(string name, int index, int main_index) {
		menu_option menu_op_temp;
		menu_op_temp.name = name;
		menu_op_temp.data = index;
		map_menu[main_index].sub_options.push_back(menu_op_temp);
	}
	void Update() {
		if (delay > 0.0f) {
			delay -= GetFrameTime(); 
			return;
		}
		auto& active_subs = map_menu[main_menu_index].sub_options;
		if (IsKeyPressed(KEY_DOWN)) {
			if (!details && !map_menu.empty()) {
				map_menu[main_menu_index].selected = false;
				main_menu_index = (main_menu_index + 1) % map_menu.size();
				map_menu[main_menu_index].selected = true;
			}
			else if (details && !active_subs.empty()) {
				active_subs[submenu_index].selected = false;
				submenu_index = (submenu_index + 1) % active_subs.size();
				active_subs[submenu_index].selected = true;
			}
		}
		if (IsKeyPressed(KEY_UP)) {
			if (!details && !map_menu.empty()) {
				map_menu[main_menu_index].selected = false;
				main_menu_index = (main_menu_index - 1 + map_menu.size()) % map_menu.size();
				map_menu[main_menu_index].selected = true;
			}
			else if (details && !active_subs.empty()) {
				active_subs[submenu_index].selected = false;
				submenu_index = (submenu_index - 1 + active_subs.size()) % active_subs.size();
				active_subs[submenu_index].selected = true;
			}
		}

		if (IsKeyPressed(KEY_ENTER)) {
			if (!details) {
				if (!map_menu.empty()) {
					sent_id = map_menu[main_menu_index].data;

					if (!active_subs.empty()) {
						details = true;
						submenu_index = 0;
						for (auto& sub : active_subs) sub.selected = false;
						active_subs[0].selected = true;
					}
					else {
						selected = true; 
					}
				}
			}
			else {
				if (!active_subs.empty()) {
					sent_id_sm = active_subs[submenu_index].data;
					selected_sm = true;
					selected = true;
					details = false; 
				}
			}
		}
		
	}
	void Draw(int windowWidth, int windowHeight, string menu_name) {
		int text_width = MeasureText(TextFormat("Select a %s",menu_name.c_str()), 50);
		DrawText(TextFormat("Select a %s", menu_name.c_str()), windowWidth / 2 - text_width / 2, windowHeight / 3, 50, YELLOW);
		Vector2 mousePos = GetMousePosition();
		for (size_t i = 0; i < map_menu.size(); i++) {
			button button(windowWidth / 2 - 125, windowHeight / 2 + 30 * i, 250, 25, map_menu[i].name.c_str(), 20);
			button.Draw(mousePos, map_menu[i].selected);
			if (button.IsPressed(mousePos)) {
				main_menu_index = i;
				sent_id = map_menu[i].data;
				if (!map_menu[i].sub_options.empty()) {
					details = true;
					submenu_index = 0;
				}
				else {
					selected = true;
				}
			}
		}
		if (details) {
			auto& active_subs = map_menu[main_menu_index].sub_options;
			int sub_x = windowWidth / 2 + 130;
			int sub_y = windowHeight / 2;
			if (active_subs[0].name == "toggle") {
				button button(windowWidth / 2 + 130, windowHeight / 2 + 30, 25, 25, "", 20);
				button.Draw(mousePos, active_subs[0].selected);
				if (button.IsPressed(mousePos)) {
					submenu_index = 0;
					sent_id_sm = active_subs[0].data;
					selected_sm = true;
					selected = true;
					details = false;
				}
			}
			else {
				for (size_t i = 0; i < active_subs.size(); i++) {
					button button(windowWidth / 2 + 130, windowHeight / 2 + 30 * i, 250, 25, active_subs[i].name.c_str(), 20);
					button.Draw(mousePos, active_subs[i].selected);
					if (button.IsPressed(mousePos)) {
						submenu_index = i;
						sent_id_sm = active_subs[i].data;
						selected_sm = true;
						selected = true;
						details = false;
					}
				}
			}
		}
	}

	bool IsConfirmed() const { return selected && sent_id != -1; }
	bool confirm_sm()const { return selected_sm && sent_id_sm != -1; }
	bool entered()const { return details; }
	int get_data() const { return sent_id; }
	int get_data_sm() const { return sent_id_sm; }
	void Reset() {
		selected = false;
		sent_id = -1;
		delay = 0.269;
	}
	void reset_sm() {
		selected_sm = false;
		sent_id_sm = -1;
	}
};
void draw_shape_menu(int shape_id, int window_height, int window_width,int x, int y){
	switch (shape_id) {
	case 0: {
		DrawRectangle(x - window_height /4, y - window_height / 4, window_height/2, window_height/2, RED);
		break;
	}
	case 1: {
		DrawCircle(x, y, window_height/4, RED);
		break;
	}
	case 2: {
		DrawSuperellipse(x, y, window_height / 4, window_height / 4, 4, RED);
		break;
	}
	}

}
class shape_card {
public:
	int x, y, height, width, id;
	Rectangle card;
	shape_card(int shape_id, int width_ceva, int h, int pos_x, int pos_y) {
		x = pos_x;
		y = pos_y;
		height = h;
		id = shape_id;
		width = width_ceva;
		card = { (float)x,(float)y,(float)width,(float)height };
	}
	void draw() {
		DrawRectangleRounded(card, 0.2, 255, BLUE);
		draw_shape_menu(id, 1.4*height, 1.4*width, x + width / 2, y + height / 2.3-8);
		DrawText(shapes[id].name.c_str(), x + width / 2 - MeasureText(shapes[id].name.c_str(), 40)/2,y+height-50, 40, RAYWHITE);
	}
	bool is_pressed() {
		return CheckCollisionPointRec(GetMousePosition(), card) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	}
	int get_id() {
		return id;
	}
};

enum class WaveType { SINE, SAWTOOTH, TRIANGLE, CHIRP };

void draw_background(int window_width, int window_height) {
	// Fill the screen with a clean, dark base canvas first
	ClearBackground(Color{ 15, 15, 25, 255 });

	float time = GetTime();

	// Define the sequence of waves we want to render down the screen
	WaveType wave_stack[] = {
		WaveType::SINE,
		WaveType::SAWTOOTH,
		WaveType::TRIANGLE,
		WaveType::CHIRP,
		WaveType::SINE,
		WaveType::SAWTOOTH,
		WaveType::TRIANGLE,
		WaveType::CHIRP,
		WaveType::SINE,
		WaveType::SAWTOOTH,
	};
	int total_waves = 10;

	float vertical_spacing = window_height / (float)(total_waves + 1);

	float frequency = 0.1f;
	float speed = 2.69f;
	float amplitude = window_height / 40.0f; 
	for (int w = 0; w < total_waves; w++) {
		std::vector<Vector2> points;
		float y_center = vertical_spacing * (w + 1); 
		WaveType current_type = wave_stack[w];

		for (int x = 0; x < window_width; x += 2) {
			float phase = (x * frequency) + (time * speed);
			float y = y_center;

			switch (current_type) {
			case WaveType::SINE:{
				y += std::sin(phase) * amplitude;
				break;
			}
			case WaveType::SAWTOOTH: {
				float phase2 = (x * (frequency * 1.2f)) + (time * (speed * 1.69));
				y += (std::sin(phase) + std::sin(phase2)) * (amplitude * 0.5f);
				break;
			}

			case WaveType::TRIANGLE: {
				float frac = fmodf(phase, 2.0f * PI) / (2.0f * PI);
				if (frac < 0.0f) frac += 1.0f;
				y += amplitude * (4.0f * std::abs(frac - 0.5f) - 1.0f);
				break;
			}

			case WaveType::CHIRP: {
				float phase2 = (x * (frequency * 2.2f)) + (time * (speed *1.69 )); 
				 y  += (std::sin(phase) + std::cos(phase2)) * (amplitude * 0.5f);
				break;
			}
			}

			points.push_back({ (float)x, y });
		}
		for (size_t i = 0; i < points.size() - 1; i++) {
			float progress = (float)i / points.size();
			float hue = fmodf((progress * 360.0f) + (time * 50.0f) + (w * 360.0f), 360.0f);

			Color color = ColorFromHSV(hue, 0.85f, 0.9f);
			color.a = 130;

			DrawLineEx(points[i], points[i + 1], 4.0f, color);
		}
	}
}
void draw_shape_screen(int win_h, int win_w, int id) {
	draw_background(win_w, win_h);
	DrawText(shapes[id].name.c_str(), win_w / 3 - MeasureText(shapes[id].name.c_str(), 100) / 2+10, 10, 100, RAYWHITE);
	draw_shape_menu(id, win_h, win_w, win_w / 3 + 10, win_h / 2);
	Rectangle stats = { (float)win_w - win_w / 3.0f - 10, 10, (float)win_w / 3.0f,(float)win_h - 20 };
	DrawRectangleRounded(stats, 0.1, 255, BLUE);
	string ceva = "Level " + std::to_string(shapes[id].level+1);
	DrawText(ceva.c_str(), win_w - win_w / 6 - MeasureText(ceva.c_str(), 60) / 2-10, 20, 60, RAYWHITE);
	ceva = "Health: " + std::to_string(shapes[id].levels[shapes[id].level].health);
	DrawText(ceva.c_str(), win_w - win_w / 6 - MeasureText(ceva.c_str(), 60) / 2-10, 20+80, 60, RAYWHITE);
	ceva = "Damage: " + std::to_string(shapes[id].levels[shapes[id].level].ammo_damage);
	DrawText(ceva.c_str(), win_w - win_w / 6 - MeasureText(ceva.c_str(), 60) / 2-10, 20 + 180, 60, RAYWHITE);
}
int main() {
	bool active = true;
	int my_id=-1;
	std::vector<player> players;
	std::vector<projectile> projectiles;
	std::optional<WorldStatePacket> world_buffer[3]{};
	std::optional<double> time_buffer[3]{};
	menu_option menu[4];
	string menu_names[4] = {"Play","Change Shape", "Change Maps", "Settings"};
	for (int i = 0; i < 4; i++) menu[i].name = menu_names[i];
	menu[0].selected = true;
	settings settings;
	settings.load();
	settings.load_shapes();
	for (int i = 0; i < shapes_unlocked.size(); i++) {
		shapes[i].level = shapes_unlocked[i].level;
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
	map current_map;

	GameState current_state = GameState::STATE_MENU;

	
	auto file_name = maps.find(settings.last_map_id);
	if (file_name != maps.end()) {
		if (!current_map.load_file(file_name->second)) {
			cout << "No map found in file" << std::endl;
			std::cin.get();
			return -1;
		}
		else {
			cout << "map loaded succesfully, width " << current_map.width << " height " << current_map.height << std::endl;
			map_select id;
			id.map_id = settings.last_map_id;
			current_map.id = settings.last_map_id;
			std::cout << "deliver map id " << id.map_id << std::endl;
			ENetPacket* packet = enet_packet_create(&id, sizeof(map_select), ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet);
		}
	}
	for (int i = 0; i < current_map.pl_pos.size(); i++) {
		player pl_temp(1000, "Player" + std::to_string(i), current_map.pl_pos[i][1], current_map.pl_pos[i][0], i);
		pl_temp.active = true;
		players.push_back(pl_temp);
	}
	float current_scale = settings.scale;
	int max_view_x = current_map.width * current_map.scale/ 2;
	int max_view_y = current_map.width * current_map.scale / 2 / 16 * 9;
	int virtualWidth = 2 * max_view_x * current_scale;
	int virtualHeight = 2 * max_view_y * current_scale;
	int pl_width_og=pl_width;
	pl_width *= current_scale;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	int win_width = settings.resolution_width;
	int win_height = settings.resolution_height;
	
	InitWindow(win_width, win_height, "Geomatrix");
	SetTargetFPS(settings.targetFPS);
	RenderTexture2D canvas = LoadRenderTexture(virtualWidth, virtualHeight);
	SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

	int players_waiting=0;
	int my_result = 0;
	int my_shape_id=settings.last_shape_id;
	int my_shape_level=0;
	int trophies_got = 0;
	int coins_got = 0;
	int imaginaries_got = 0;
	int shape_screen_id = 0;
	menu_std map_menu;
	for (auto it : maps_o) {
		map_menu.add_option_main(it.first, it.second);
	}
	menu_std settings_menu;
	for (int i = 0; i < 4; i++) {
		settings_menu.add_option_main(settings_array[i], i);
	}
	int monitor = GetCurrentMonitor();
	int nativeW = GetMonitorWidth(monitor);
	int nativeH = GetMonitorHeight(monitor);
	for (int i = 0; i <7; i++) {
		if ((nativeW >= resolutions_matrix[i][0]) && (nativeH >= resolutions_matrix[i][1])) {
			settings_menu.add_option_sub(std::to_string(resolutions_matrix[i][0]) + "x" + std::to_string(resolutions_matrix[i][1]), i,0);
		}
	}
	if (settings.fullscreen) {
		ToggleFullscreen();
	}
	settings_menu.add_option_sub("toggle", 0, 1);
	for (int i = 8; i >= 0; i--) {
		int refresh_rate = GetMonitorRefreshRate(GetCurrentMonitor());
		if (refresh_rate >= FPS_array[i]) {
			settings_menu.add_option_sub(std::to_string(FPS_array[i])+"Hz", i, 2);
		}
	}
	for (int i = 5; i >= 0; i--) {
		settings_menu.add_option_sub(TextFormat("%.1f", scale_array[i]), i, 3);
	}
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
							<< expected_bytes << ", got " << event.packet->dataLength << std::endl;
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
							players[p_id].shape.id = incoming_world.players[i].shape_id;
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
						trophies_got = event.packet->data[3];
						coins_got = event.packet->data[4];
						settings.quanta += trophies_got;
						settings.reals += coins_got;
						settings.save();

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
			draw_background(windowWidth, windowHeight);
			bool selected = false;
			int option = -1;
			Vector2 mousePos = GetMousePosition();
			DrawText("Geomatrix", windowWidth / 2 - MeasureText("Geomatrix", 80) /2, windowHeight / 13, 80, YELLOW);
			string map_name = maps_id_name.find(current_map.id)->second + " map";

			for (int i = 0; i < 4; i++) {
				button button(windowWidth / 2-125, windowHeight / 2 + 30 * i,250,25, menu[i].name.c_str(), 20);
				switch (i) {
				case 0: {
					button.bounds = { (float)windowWidth - 10 - windowWidth / 3.69f,(float) windowHeight-10-windowHeight/8, (float)windowWidth / 3.69f,(float)windowHeight / 8 };
					button.fontSize = 60;
					button.hoverColor = Color{ 215,215,0,255 };
					button.corner_r = 0.4;
					break;
				}
				case 1: {
					button.bounds = { (float) 10,(float)windowHeight - 10 - windowHeight / 8, (float)windowWidth / 3.69f,(float)windowHeight / 8 };
					button.fontSize = 40;
					button.hoverColor = Color{ 215,215,0,255 };
					button.corner_r = 0.4;
					break;
				}
				case 2: {
					button.bounds = { (float)windowWidth / 3.69f+20,(float)windowHeight - 10 - windowHeight / 8, (float)windowWidth- 2* windowWidth / 3.69f-40,(float)windowHeight / 8 };
					button.fontSize = 40;
					button.hoverColor = Color{ 215,215,0,255 };
					button.corner_r = 0.4;
					button.text = map_name.c_str();

					break;
				}
				case 3:{
					button.bounds = { (float)windowWidth-10- windowWidth / 5.69f,(float)10, (float)windowWidth / 5.69f,(float)windowHeight / 12 };
					button.fontSize = 40;
					button.hoverColor = Color{ 215,215,0,255 };
					button.corner_r = 0.4;
					break;
				}
				default:{
					break;
				}
				}



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
			
			DrawRectangleRounded(Rectangle{(float)windowWidth-windowWidth/5.69f - 260, 10, 240,60},0.2,255,RED);
			DrawText(TextFormat("%d reals", settings.reals), (float)windowWidth - windowWidth / 5.69f - 140- MeasureText(TextFormat("%d reals", settings.reals),40)/2, 20, 40,RAYWHITE );
			DrawRectangleRounded(Rectangle{ 10, 10, 280,60 }, 0.2, 255, RED);
			DrawText(TextFormat("%d quanta", settings.quanta), (float)10+280/2.0f- MeasureText(TextFormat("%d quanta", settings.quanta), 40) / 2, 20, 40, RAYWHITE);


			Rectangle bounds = {windowWidth-300,windowHeight-150,200,100};
			draw_shape_menu(settings.last_shape_id, windowHeight, windowWidth, windowWidth/2,windowHeight/2);
			if (settings.tutorial) {
				int pop_w = windowWidth / 8 * 5;
				int pop_h = windowHeight / 8 * 5;
				int pop_x = (windowWidth - windowWidth / 8 * 5) / 2;
				int pop_y = (windowHeight - windowHeight / 8 * 5) / 2;
				Rectangle pop_up_rec = {pop_x,pop_y,pop_w,pop_h};
				DrawRectangleRounded(pop_up_rec, 0.0569, 1024, BLUE);
				button pop_exit((windowWidth + windowWidth / 8 * 5) / 2-55,pop_y+10, 45,45,"X",35);
				pop_exit.corner_r = .4;
				if (pop_exit.IsPressed(GetMousePosition())) {
					settings.tutorial = false;
					settings.save();
				}

				pop_exit.Draw(GetMousePosition(), false);

				string ceva = "Controls";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 50) / 2, pop_y + 20, 50, WHITE);
				ceva = "W A S D for moving";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, pop_y+90, 20, RAYWHITE);
				ceva = "Arrow keys for aiming";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, pop_y+120, 20, RAYWHITE);
				ceva = "press E to shoot";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, pop_y+150, 20, RAYWHITE);
				ceva = "Useful controls for testing";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 30) / 2, windowHeight/2, 30, RAYWHITE);
				ceva = "press Z to go back to the start menu";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, windowHeight/2 + 50, 20, RAYWHITE);
				ceva = "press x to exit the game client";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, windowHeight / 2 + 80, 20, RAYWHITE);
				ceva = "press B to enter a match without connecting the total number of players";
				DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, windowHeight / 2 + 110, 20, RAYWHITE);
			}
			DrawFPS(10, 10);
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
					play.shape_id = my_shape_id;
					play.shape_level = 2;
					ENetPacket* packet = enet_packet_create(&play, sizeof(pressed_play), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					break;
				}
				case 1: {
					current_state = GameState::state_shape_select;
					break;
				}
				case 2: {
					current_state = GameState::state_map_select;
					break;
				}
				case 3: {
					current_state = GameState::state_settings_menu;
					break;
				}
				default:
					break;
				}
			}

			break;
		}
		case GameState::state_map_select: {
			Vector2 mousePos = GetMousePosition();
			map_menu.Update();
			if (map_menu.IsConfirmed()) {
				map_select id;
				id.map_id = map_menu.get_data();
				std::cout << "deliver map id " << id.map_id << std::endl;
				ENetPacket* packet = enet_packet_create(&id, sizeof(map_select), ENET_PACKET_FLAG_RELIABLE);
				map_menu.Reset();
				enet_peer_send(peer, 0, packet);
				current_state = GameState::STATE_MENU;
				auto file_name = maps.find(id.map_id);
				if (file_name != maps.end()) {
					if (!current_map.load_file(file_name->second)) {
						cout << "No map found in file" << std::endl;
						std::cin.get();
					}
					else {
						cout << "map loaded succesfully, width " << current_map.width << " height " << current_map.height << std::endl;
						max_view_x = current_map.width * current_map.scale / 2;
						max_view_y = current_map.width * current_map.scale / 2 / 16 * 9;
						virtualWidth = 2 * max_view_x * current_scale;
						virtualHeight = 2 * max_view_y * current_scale;
						players.clear();
						for (auto& item : world_buffer) {
							item = std::nullopt; 
						}
						for (int i = 0; i < current_map.pl_nr; i++) {
							player pl_temp(1000, "Player" + std::to_string(i), current_map.pl_pos[i][1], current_map.pl_pos[i][0], i);
							pl_temp.active = true;
							players.push_back(pl_temp);
						}
						settings.last_map_id = id.map_id;
						current_map.id = id.map_id;
						settings.save();
						UnloadRenderTexture(canvas);
						canvas = LoadRenderTexture(virtualWidth, virtualHeight);
						SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
					}
				}
				
			}	
			button back(10, 10, windowWidth / 10, windowHeight / 12, "Back", 30);

			if (back.IsPressed(GetMousePosition())) {
				current_state = GameState::STATE_MENU;
			}
			BeginDrawing();
			draw_background(windowWidth, windowHeight);
			back.Draw(GetMousePosition(), false);


			map_menu.Draw(windowWidth, windowHeight, "map");
			EndDrawing();
			if (IsKeyPressed(KEY_Z)) {
				current_state = GameState::STATE_MENU;
			}

			break;
		}
		case GameState::state_shape_select: {
			button back(10, 10, windowWidth / 10, windowHeight / 12, "Back", 30);
			if (back.IsPressed(GetMousePosition())) {
				current_state = GameState::STATE_MENU;

			}
			BeginDrawing();
			draw_background(windowWidth, windowHeight);

			back.Draw(GetMousePosition(), false);
			int row_nr=0;
			for(int i=0;i< std::size(shapes);i++){
				int order = i % 3;
				int pos_x = 0;
				int pos_y = 40 + (windowHeight / 4+10) * row_nr;
				switch (order) {
				case 0: {
					pos_x = windowWidth / 2 - windowWidth / 8 - 10 - windowWidth / 4;
					break;
				}
				case 1: {
					pos_x = windowWidth / 2 - windowWidth / 8;
					break;
				}
				case 2: {
					pos_x = windowWidth / 2 + windowWidth / 8 + 10;
					row_nr++;
					break;
				}
				default: {
					break;
				}
				}
				shape_card card(i, windowWidth / 4, windowHeight / 4, pos_x, pos_y);
				card.draw();
				if (card.is_pressed()) {
					current_state = GameState::state_shape_screen;
					shape_screen_id = card.get_id();
				}
			}
			
			EndDrawing();


			if (IsKeyPressed(KEY_Z)) {
				current_state = GameState::STATE_MENU;
			}
			break;
		}
		case GameState::state_shape_screen: {
			bool unlocked = false;
			int index = 0;
			for (int i = 0; i < std::size(shapes_unlocked); i++) {
				if (shapes_unlocked[i].id == shape_screen_id) {
					unlocked = true;
					index = i;
					break;
				}
			}

			button select(windowWidth - windowWidth / 3, windowHeight - windowHeight / 6-20, windowWidth / 3-20, windowHeight / 6,TextFormat("%d reals",shapes_price_reals_array[shape_screen_id]), 50);
			select.corner_r = 0.269;
			button upgrade(windowWidth - windowWidth / 3, windowHeight - 2*windowHeight / 6 - 30, windowWidth / 3 - 20, windowHeight / 6, TextFormat("%d reals", level_up_prices[shapes[shape_screen_id].level]), 50);
			upgrade.corner_r = 0.269;
			int img_price=0;
			bool img_pop_up=false;
			if (unlocked) {
				select.text = "Select";
				bool check = settings.reals >= level_up_prices[shapes[shape_screen_id].level];
				if (!check) {
					upgrade.text_color = GRAY;
				}
				if (select.IsPressed(GetMousePosition())) {
					my_shape_id = shape_screen_id;
					my_shape_level = shapes_unlocked[index].level;
					settings.last_shape_id = shape_screen_id;
					settings.save();
					current_state = GameState::STATE_MENU;
				}
				if (upgrade.IsPressed(GetMousePosition())) {
					if (check) {
						settings.reals -= level_up_prices[shapes[shape_screen_id].level];
						shapes[shape_screen_id].level++;
						shapes_unlocked[index].level++;
						my_shape_level++;
						settings.save_shape();
					}
					
					
				}
			}
			else {
				bool check = settings.reals >= shapes_price_reals_array[shape_screen_id];
				if (!check) {
					select.text_color = GRAY;
				}
				if (select.IsPressed(GetMousePosition())) {
					if (check) {
						settings.reals -= shapes_price_reals_array[shape_screen_id];
						settings.save();
						shapes_unlocked.push_back(shapes[shape_screen_id]);
						settings.save_shape();
					}
					//Another currency (imaginaries) work in progress
					//else {
					//	check = settings.imaginaries >= shapes_price_imaginaries_array[shape_screen_id];
					//	if (check) {
					//		settings.imaginaries -= shapes_price_imaginaries_array[shape_screen_id];
					//		settings.save();
					//		shapes_unlocked.push_back(shapes[shape_screen_id]);
					//		settings.save_shape();
					//	}
					//}
				}
			}
			button back(10, 10, windowWidth / 10, windowHeight / 12, "Back", 30);

			if (back.IsPressed(GetMousePosition())) {
				current_state = GameState::state_shape_select;

			}
			BeginDrawing();

			draw_shape_screen(windowHeight, windowWidth, shape_screen_id);
			select.Draw(GetMousePosition(), false);
			back.Draw(GetMousePosition(), false);
			if (unlocked&&(shapes[shape_screen_id].level<2)) {
				upgrade.Draw(GetMousePosition(), false);
			}
			EndDrawing();
			if (IsKeyPressed(KEY_Z)) {
				current_state = GameState::STATE_MENU;
			}
			break;
		}
		case GameState::state_settings_menu: {
			settings_menu.Update();
			int setting_selected = settings_menu.get_data();
			button back(10, 10, windowWidth / 10, windowHeight / 12, "Back", 30);

			if (back.IsPressed(GetMousePosition())) {
				current_state = GameState::STATE_MENU;

			}
			BeginDrawing();
			ClearBackground(DARKBLUE);
			draw_background(windowWidth, windowHeight);
			back.Draw(GetMousePosition(), false);
			settings_menu.Draw(windowWidth, windowHeight, "setting");
			if (settings_menu.entered()) {
				switch (setting_selected) {
				case 0: {
					if (settings_menu.confirm_sm()) {
						int index = settings_menu.get_data_sm();
						int new_width = resolutions_matrix[index][0];
						int new_height = resolutions_matrix[index][1];
						SetWindowSize(new_width, new_height);
						settings.resolution_width = new_width;
						settings.resolution_height = new_height;
						settings.save();
						int monitor = GetCurrentMonitor();
						int monitor_w = GetMonitorWidth(monitor);
						int monitor_h = GetMonitorHeight(monitor);
						int center_x = (monitor_w - new_width) / 2;
						int center_y = (monitor_h - new_height) / 2;
						SetWindowPosition(center_x, center_y);
						settings_menu.reset_sm();

					}
					break;
				}
				case 1:{
					if (settings.fullscreen) {
						DrawCircle(windowWidth / 2 + 142.5f, windowHeight / 2 + 42.5f, 10, BLUE);
					}
					if (IsKeyPressed(KEY_ENTER)) {
						if (settings.fullscreen) {
							settings.fullscreen = false;
							ToggleFullscreen();

						}
						else {
							settings.fullscreen = true;
							ToggleFullscreen();
						}
						settings.save();
					}

					break;
				}
				case 2: {
					if (settings_menu.confirm_sm()) {
						int index = settings_menu.get_data_sm();
						SetTargetFPS(FPS_array[index]);
						settings.targetFPS = FPS_array[index];
						settings.save();
						settings_menu.reset_sm();
					}
					break;
				}
				case 3: {
					if (settings_menu.confirm_sm()) {
						int index = settings_menu.get_data_sm();
						current_scale = scale_array[index];
						virtualWidth = 2 * max_view_x * current_scale;
						virtualHeight = 2 * max_view_y * current_scale;
						pl_width =pl_width_og* current_scale;
						UnloadRenderTexture(canvas);
						canvas = LoadRenderTexture(virtualWidth, virtualHeight);
						SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
						settings.scale = current_scale;
						settings.save();
						settings_menu.reset_sm();
					}

					break;
				}
				default: {
					break;
				}
				}
			}

			string ceva = "Press Enter twice to confirm changes";
			DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, windowHeight / 1.35, 20, RAYWHITE);
			ceva = "Press Z to go back";
			DrawText(ceva.c_str(), windowWidth / 2 - MeasureText(ceva.c_str(), 20) / 2, windowHeight / 1.35+30, 20, RAYWHITE);
			DrawFPS(10, 10);
			EndDrawing();
			if (IsKeyPressed(KEY_Z)) {
				settings_menu.Reset();
				settings_menu.reset_sm();
				current_state = GameState::STATE_MENU;

			}


			break;
		}
		case GameState::STATE_lobby: {
			BeginDrawing();
			draw_background(windowWidth, windowHeight);

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
		case GameState::STATE_PLAYING:{
			player_input input = movement_float(IsGamepadAvailable(0));
			if (my_id != -1) {
				SendInputToServer(peer, input);
			}
				match_drawing(world_buffer, time_buffer, players, current_map, canvas, my_id, virtualWidth, virtualHeight, max_view_x, max_view_y,windowWidth,windowHeight,current_scale);
				if (IsKeyPressed(KEY_Z)) {
					current_state = GameState::STATE_MENU;

					disconnect ceva;
					ENetPacket* packet = enet_packet_create(&ceva, sizeof(disconnect), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
				}
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
			draw_background(windowWidth, windowHeight);
			DrawText("GAME OVER!", windowWidth / 2 - 160, windowHeight / 3, 30, WHITE);
			DrawText(TextFormat("Your rank: %d", my_result), windowWidth / 2 - 160, windowHeight / 2.5, 30, WHITE);
			DrawText(TextFormat("Got: %d quanta and %d reals", trophies_got, coins_got), windowWidth / 2 - 160, windowHeight / 2.25, 30, WHITE);
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
