#include "CMakeProject1.h"
#include "enet/enet.h"

using std::cout;
using std::string;
//network stuff begin
#pragma pack(push, 1)
#define MAX_PLAYERS 10
#define MAX_PROJECTILES 100
#define PACKET_SIZE 884
enum class PacketType : uint8_t {
	ID_ASSIGNMENT = 0,
	WORLD_STATE = 1,
	MATCH_STATE_CHANGE = 2,
	LOBBY_COUNT = 3,
	FORCE_CONNECT = 4,
	press_play=5,
	input =6,
	disconnect = 7,
	select_map= 8,
};
struct MatchStatePacket {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::MATCH_STATE_CHANGE);
	uint8_t new_state;
	uint8_t position;
	int8_t trophies;
	uint8_t coins;
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
struct id_assign {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::ID_ASSIGNMENT);
	uint8_t id{};
};
struct lobby_count {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::LOBBY_COUNT);
	uint8_t count{};
};
struct player_input {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::input);
	float x{};
	float y{};
	bool attack{};
	float aim_x{};
	float aim_y{};
};
#pragma pack(pop)

//network stuff end
enum class ServerState {
	LOBBY,               
	MATCH_IN_PROGRESS,  
	GAME_OVER            
};
std::unordered_map<int, string>maps = {
	{0, "maps/default_map.txt"},
	{1, "maps/ceva.txt"}
};
int trophy_payout[10] = {26,18,13,9,6,2,-2,-6,-9,-11};
int coin_payout[10] = { 100,79,69,50,38,26,21,16,10,6};
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
		file >> height >> width >> scale>>pl_nr;
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
	int speed{};
	int range{};
	int ammo{};
	int ammo_r{};
	int ammo_speed{};
	int ammo_damage{};
	float reload_time{};
	level_stats levels[3];
};
shape_ch shapes[3] = {
	{
		.id = 0,
		.name= "Square",
		.speed = 458,
		.range = 400,
		.ammo = 12,
		.ammo_r = 26,
		.ammo_speed = 400,
		.reload_time = 0.5f,
		.levels = {
			{.health = 1000, .ammo_damage =100 },
			{.health = 1200, .ammo_damage = 120 },
			{.health = 1450, .ammo_damage = 145 },
		}
	},
	{
		.id = 1,
		.name = "Circle",
		.speed = 364,
		.range = 300,
		.ammo = 6,
		.ammo_r = 56,
		.ammo_speed = 500,
		.reload_time = 0.4f,
		.levels = {
			{.health = 1500, .ammo_damage = 65 },
			{.health = 1700, .ammo_damage = 80 },
			{.health = 1950, .ammo_damage = 105 },
		}
	},
	{
		.id = 2,
		.name = "Superellipse",
		.speed = 456,
		.range = 600,
		.ammo = 26,
		.ammo_r = 26,
		.ammo_speed = 500,
		.reload_time = 0.5f,
		.levels = {
			{.health = 1250, .ammo_damage = 150 },
			{.health = 1450, .ammo_damage = 170 },
			{.health = 1800, .ammo_damage = 200 },
		}
	}
};



const int pl_width = 96;
struct player {
	int id{};
	string name;
	shape_ch shape;
	int shape_level{};
	int health{};
	int ammo_now{};
	float reload_timer{};
	float shoot_delay{};
	int pos_x{};
	int pos_y{};
	int new_x{};
	int new_y{};
	float pos_f_x{};
	float pos_f_y{};
	float joy_x{};
	float joy_y{};
	float aim_x{};
	float aim_y{};
	bool attack = false;
	bool active = true;
	bool connected = false;
	ENetPeer* peer{ nullptr };
};

struct projectile {
	int id{};
	int pr_id{};
	int radius{};
	int speed{};
	int pos_x{};
	int pos_y{};
	int new_x{};
	int new_y{};
	int range{};
	int damage{};
	float distance{};
	float pos_f_x{};
	float pos_f_y{};
	float aim_x{};
	float aim_y{};
	projectile(int r,int s,int range, int damage):
		radius(r),
		speed(s),
		range(range),
		damage(damage)
	{}
};



bool collision(map& map, int& new_x, int& new_y) {
	int corners_x[4] = { new_x - pl_width / 2, new_x + pl_width / 2 - 1, new_x - pl_width / 2, new_x + pl_width / 2 - 1 };
	int corners_y[4] = { new_y - pl_width / 2, new_y - pl_width / 2, new_y + pl_width / 2 - 1, new_y + pl_width / 2 - 1 };

	for (int i = 0; i < 4; i++) {
		int tile_x = corners_x[i] / map.scale;
		int tile_y = corners_y[i] / map.scale;

		if (tile_x < 0 || tile_x >= map.width || tile_y < 0 || tile_y >= map.height) return false;

		int tile_type = map.matrix[tile_y][tile_x];
		if (tile_type != 0 && tile_type != 2) {
			return false;
		}
	}
	return true;
}
bool aim(map& map, player(&players)[MAX_PLAYERS], projectile& pr, float d_time) {
	float lengthSq = pr.aim_x * pr.aim_x + pr.aim_y * pr.aim_y;
	float distance_min = pl_width / 2 + pr.radius;
	if (lengthSq > 1.0f) {
		float length = std::sqrt(lengthSq);
		pr.aim_x /= length; // Scale X down (becomes ~0.707) if needed, cause it is a little redundant
		pr.aim_y /= length; // Scale Y down (becomes ~0.707) if needed, cause it is a little redundant
	}
	if((pr.aim_x==0)&&(pr.aim_y==0)) return false;
	pr.pos_f_x += pr.aim_x * pr.speed * d_time;
	pr.pos_f_y += pr.aim_y * pr.speed * d_time;
	pr.new_x = static_cast<int>(pr.pos_f_x);
	pr.new_y = static_cast<int>(pr.pos_f_y);
	if (pr.distance >= pr.range) {
		return false;
	}
	int* tile = &map.matrix[pr.new_y/map.scale][pr.new_x/map.scale];

	if ((*tile == 3) || (*tile == 4)) {
		return false;
	}
	pr.distance += std::sqrt(pow(pr.new_x - pr.pos_x, 2) + pow(pr.new_y - pr.pos_y, 2));
	pr.pos_x = pr.new_x;
	pr.pos_y = pr.new_y;
	for (int i = 0; i < map.pl_nr; i++) {
		if ((pr.id != i)&&players[i].active) {
			if ((abs(players[i].pos_x - pr.pos_x) <= distance_min) &&(abs(players[i].pos_y - pr.pos_y) <= distance_min)) {
				players[i].health -= pr.damage;
				if (players[i].health <= 0) {
					players[i].active = false;
				}
				return false;
			}
		}
	}
	return true;
}

void movement_float(map& map, player& pl, double d_time) {
	float lengthSq = pl.joy_x * pl.joy_x + pl.joy_y * pl.joy_y;
	if (lengthSq > 1.0f) {
		float length = std::sqrt(lengthSq);
		pl.joy_x /= length; // Scale X down (becomes ~0.707) if needed, cause it is a little redundant
		pl.joy_y /= length; // Scale Y down (becomes ~0.707) if needed, cause it is a little redundant
	}
	pl.pos_f_x += pl.joy_x * pl.shape.speed * d_time;
	pl.pos_f_y += pl.joy_y * pl.shape.speed * d_time;
	pl.new_x = static_cast<int>(pl.pos_f_x);
	pl.new_y = static_cast<int>(pl.pos_f_y);

	if (pl.new_x != pl.pos_x) {
		if (collision(map, pl.new_x, pl.pos_y)) { 
			pl.pos_x = pl.new_x;
		}
		else {
			pl.pos_f_x = static_cast<float>(pl.pos_x);
		}
	}
	if (pl.new_y != pl.pos_y) {
		if (collision(map, pl.pos_x, pl.new_y)) { 
			pl.pos_y = pl.new_y;
		}
		else {
			pl.pos_f_y = static_cast<float>(pl.pos_y);
		}
	}		
}

void simulation_pl_pr(map& map,player (&players)[MAX_PLAYERS], std::vector<projectile>& projectiles, double tick_time, int& projectile_ids) {
	for (int i = 0; i < map.pl_nr; i++) {
		if ((players[i].connected) && (players[i].active)) {
			movement_float(map, players[i], tick_time);
			if (players[i].ammo_now < players[i].shape.ammo) {
				players[i].reload_timer -= tick_time;
				if (players[i].shoot_delay > 0.0) {
					players[i].shoot_delay -= tick_time;
					if (players[i].shoot_delay <= 0.0) {
						players[i].shoot_delay = 0.0;
					}
				}
				if (players[i].reload_timer <= 0) {
					players[i].ammo_now++;
					players[i].reload_timer = 0.0;
					if (players[i].ammo_now < players[i].shape.ammo) {
						players[i].reload_timer = players[i].shape.reload_time;
					}
				}
			}

			if ((players[i].attack) && (projectiles.size() < MAX_PROJECTILES) &&
				(players[i].ammo_now > 0) &&
				((players[i].aim_x != 0.0) || (players[i].aim_y != 0.0)) &&
				(players[i].shoot_delay == 0.0)) {
				projectile pr(players[i].shape.ammo_r, players[i].shape.ammo_speed, players[i].shape.range, players[i].shape.ammo_damage);
				pr.pos_x = players[i].pos_x;
				pr.pos_y = players[i].pos_y;
				pr.pos_f_x = static_cast<float>(players[i].pos_x);
				pr.pos_f_y = static_cast<float>(players[i].pos_y);
				pr.id = players[i].id;
				pr.pr_id = projectile_ids;
				pr.aim_x = players[i].aim_x;
				pr.aim_y = players[i].aim_y;
				pr.distance = 0;
				projectile_ids++;
				projectiles.push_back(pr);
				players[i].attack = false;
				if (players[i].ammo_now == players[i].shape.ammo) {
					players[i].reload_timer = players[i].shape.reload_time;
				}
				players[i].ammo_now--;
				players[i].shoot_delay = 0.096;
			}
		}
	}
	auto it = projectiles.begin();
	while (it != projectiles.end()) {
		bool is_alive = aim(map, players, *it, tick_time);
		if (!is_alive) {
			it = projectiles.erase(it);
		}
		else {
			++it;
		}
	}
}
void network_pl_pr(map& map, player(&players)[MAX_PLAYERS], std::vector<projectile>& projectiles, int max_view_x, int max_view_y) {
	for (int i = 0; i < map.pl_nr; i++) {
		if ((players[i].connected) && (players[i].active)) {
			player* pl = &players[i];
			uint8_t send_buffer[PACKET_SIZE];

			send_buffer[0] = static_cast<uint8_t>(PacketType::WORLD_STATE);
			send_buffer[1] = 0;
			send_buffer[2] = 0;
			send_buffer[3] = pl->ammo_now;
			uint8_t* payload_dst = send_buffer + 4;


			for (int j = 0; j < map.pl_nr; j++) {
				if (players[j].active) {
					int distance_x = abs(players[j].pos_x - pl->pos_x);
					int distance_y = abs(players[j].pos_y - pl->pos_y);
					int view_y = max_view_y;
					if (pl->pos_y - max_view_y < 0) {
						view_y -= pl->pos_y - max_view_y;
					}
					else if (pl->pos_y + max_view_y > map.height * map.scale) {
						view_y += pl->pos_y + max_view_y - map.height * map.scale;
					}
					int view_x = max_view_x;
					if (pl->pos_x - max_view_x < 0) {
						view_x -= pl->pos_x - max_view_x;
					}
					else if (pl->pos_x + max_view_x > map.width * map.scale) {
						view_x += pl->pos_x + max_view_x - map.width * map.scale;
					}
					if ((distance_x <= view_x) && (distance_y <= view_y)) {
						PlayerNetworkState p_state{
							.pos_x = static_cast<uint16_t>(players[j].pos_x),
							.pos_y = static_cast<uint16_t>(players[j].pos_y),
							.health = static_cast<uint16_t>(players[j].health),
							.id = static_cast<uint8_t>(players[j].id),
							.shape_id= static_cast<uint8_t>(players[j].shape.id),
							.active = players[j].active
						};
						std::memcpy(payload_dst, &p_state, sizeof(PlayerNetworkState));

						payload_dst += sizeof(PlayerNetworkState);
						send_buffer[1]++;
					}
				}
				else if (players[j].id == pl->id) {
					PlayerNetworkState p_state{
							.pos_x = static_cast<uint16_t>(players[j].pos_x),
							.pos_y = static_cast<uint16_t>(players[j].pos_y),
							.health = static_cast<uint16_t>(players[j].health),
							.id = static_cast<uint8_t>(players[j].id),
							.active = players[j].active
					};
					std::memcpy(payload_dst, &p_state, sizeof(PlayerNetworkState));
					payload_dst += sizeof(PlayerNetworkState);
					send_buffer[1]++;

				}
			}
			auto it = projectiles.begin();
			while (it != projectiles.end()) {
				int distance_x = abs(it->pos_x - pl->pos_x);
				int distance_y = abs(it->pos_y - pl->pos_y);
				int view_y = max_view_y;
				if (pl->pos_y - max_view_y < 0) {
					view_y -= pl->pos_y - max_view_y;
				}
				else if (pl->pos_y + max_view_y > map.height * map.scale) {
					view_y += pl->pos_y + max_view_y - map.height * map.scale;
				}
				int view_x = max_view_x;
				if (pl->pos_x - max_view_x < 0) {
					view_x -= pl->pos_x - max_view_x;
				}
				else if (pl->pos_x + max_view_x > map.width * map.scale) {
					view_x += pl->pos_x + max_view_x - map.width * map.scale;
				}
				if ((distance_x <= view_x) && (distance_y <= view_y)) {
					projectile_network pr_state{
						.pos_x = static_cast<uint16_t>(it->pos_x),
						.pos_y = static_cast<uint16_t>(it->pos_y),
						.radius = static_cast<uint16_t>(it->radius),
						.id = static_cast<uint16_t>(it->pr_id)
					};
					std::memcpy(payload_dst, &pr_state, sizeof(projectile_network));
					payload_dst += sizeof(projectile_network);
					send_buffer[2]++;

				}
				++it;
			}
			int dynamic_size = sizeof(uint8_t) * 4
				+ (send_buffer[1] * sizeof(PlayerNetworkState))
				+ (send_buffer[2] * sizeof(projectile_network));
			ENetPacket* packet = enet_packet_create(&send_buffer, dynamic_size, 0);
			enet_peer_send(players[i].peer, 0, packet);
		}
	}

}
int main() {
	bool active = true;
	player players[MAX_PLAYERS];
	std::vector<projectile> projectiles;
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
	int max_view_y = current_map.height * current_map.scale / 2/16*9;
	for (int i = 0; i < current_map.pl_pos.size(); i++) {
		player pl_temp{};
		pl_temp.id = i;
		pl_temp.pos_x = current_map.pl_pos[i][1];
		pl_temp.pos_y = current_map.pl_pos[i][0];
		pl_temp.shape = shapes[0];
		pl_temp.shape_level = 0;
		pl_temp.ammo_now = pl_temp.shape.ammo;
		pl_temp.health = pl_temp.shape.levels[0].health;
		pl_temp.shape.ammo_damage = pl_temp.shape.levels[0].ammo_damage;
		pl_temp.active = true;
		players[i] = pl_temp;
	}
	if (enet_initialize() != 0) {
		std::cerr << "An error occurred while initializing ENet.\n";
		return 1;
	}
	atexit(enet_deinitialize);

	ENetAddress address;
	address.host = ENET_HOST_ANY; 
	address.port = 12345;         
	ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
	if (server == nullptr) {
		std::cerr << "An error occurred while trying to create an ENet server host.\n";
		return 1;
	}
	cout << "Server started on port 12345. Waiting for clients...\n";

	ENetEvent event;
	const double tick_rate = 69;
	const double tick_time = 1.0f / tick_rate;
	double accumulated_time = 0;
	auto last_time = std::chrono::high_resolution_clock::now();
	int projectile_ids = 0;
	int connected_count = 0;
	ServerState current_state = ServerState::LOBBY;
	while (active) {
		bool connect_anyway=false;
		while (enet_host_service(server, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				cout << "A new client connected from "
					<< (*event.peer).address.host << ":"
					<< event.peer->address.port << "\n";
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE: {
				uint8_t packet_type = event.packet->data[0];
				if (packet_type == static_cast<uint8_t>(PacketType::input)) {
					if (event.packet->dataLength == sizeof(player_input)) {
						player_input* received_input = (player_input*)event.packet->data;
						uintptr_t target_id = (uintptr_t)event.peer->data;
						for (auto& pl : players) {
							if (pl.id == target_id) {
								pl.joy_x = received_input->x;
								pl.joy_y = received_input->y;
								pl.attack = received_input->attack;
								if ((received_input->attack) && ((received_input->aim_x != 0.0) || (received_input->aim_y != 0.0))) {
									pl.aim_x = received_input->aim_x;
									pl.aim_y = received_input->aim_y;
								}
								else {
									pl.aim_x = 0;
									pl.aim_y = 0;
								}
								break;
							}
						}
					}
					else {
						std::cerr << "Received a malformed input packet of unexpected size.\n";
					}
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::FORCE_CONNECT)) {
					connect_anyway = true;
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::press_play)) {
					for (int i = 0; i < current_map.pl_nr; i++) {
						if (!players[i].connected) {
							players[i].connected = true;
							players[i].active = true;
							players[i].peer = event.peer;
							players[i].id = i;
							event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(i));
							uint8_t shape_id = static_cast<uint8_t>(event.packet->data[1]);
							uint8_t level = static_cast<uint8_t>(event.packet->data[2]);
							players[i].shape = shapes[shape_id];
							players[i].shape_level = level;
							players[i].ammo_now = players[i].shape.ammo;
							players[i].health = shapes[shape_id].levels[level].health;
							players[i].shape.ammo_damage = shapes[shape_id].levels[level].ammo_damage;
							cout << "new shape loaded for player " << players[i].id << "with the shape id: " << shape_id << "level" << level << std::endl;
							id_assign id_sent;
							id_sent.id = i;
							ENetPacket* packet = enet_packet_create(&id_sent, sizeof(id_assign), 0);
							enet_peer_send(event.peer, 0, packet);
							cout << "Player " << i << " joined the lobby." << std::endl;
							break;
						}
					}
				}
				else if (packet_type == static_cast<uint8_t>(PacketType::disconnect)) {
					cout << "received message" << std::endl;
					for (int i = 0; i < current_map.pl_nr; i++) {
						if (players[i].peer == event.peer) {
							cout << "Player " << players[i].id << " disconnected." << std::endl;
							players[i].peer = nullptr;
							players[i].connected = false;
							break;
						}
					}

				}
				else if (packet_type == static_cast<uint8_t>(PacketType::select_map)) {
					uint8_t id = static_cast<uint8_t>(event.packet->data[1]);
					auto file_name = maps.find(id);
					if (file_name != maps.end()) {
						if (!current_map.load_file(file_name->second)) {
							cout << "No map found in file" << std::endl;
						}
						else {
							cout << "map loaded succesfully, width " << current_map.width << " height " << current_map.height << std::endl;
							max_view_x = current_map.width * current_map.scale / 2;
							max_view_y = current_map.height * current_map.scale / 2 / 16 * 9;
							for (int i = 0; i < current_map.pl_pos.size(); i++) {
								player pl_temp{};
								pl_temp.id = i;
								pl_temp.pos_x = current_map.pl_pos[i][1];
								pl_temp.pos_y = current_map.pl_pos[i][0];
								pl_temp.shape = shapes[0];
								pl_temp.shape_level = 0;
								pl_temp.ammo_now = pl_temp.shape.ammo;
								pl_temp.health = pl_temp.shape.levels[0].health;
								pl_temp.shape.ammo_damage = pl_temp.shape.levels[0].ammo_damage;
								pl_temp.active = true;
								players[i] = pl_temp;
							}
						}
					}
				}
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: {
				if (event.peer->data != nullptr) {
					uintptr_t disconnected_player = reinterpret_cast<uintptr_t>(event.peer->data);
					cout << "Player " << disconnected_player << " disconnected." << std::endl;
					for (int i = 0; i < current_map.pl_nr;i++) {
						if (players[i].peer == players[disconnected_player].peer) {
							players[i].connected=false;
							players[i].peer = nullptr;
							break; 
						}
					}
				}
				if ((connected_count == 0)&&(current_state==ServerState::MATCH_IN_PROGRESS)) {
					current_state = ServerState::GAME_OVER;
				}
				break;
			}

			case ENET_EVENT_TYPE_NONE:
				break;
			}
		}
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = current_time - last_time;
		last_time = current_time;
		accumulated_time += elapsed.count();
		while (accumulated_time >= tick_time){	
			if (current_state == ServerState::LOBBY) {
				int connected_d = 0;
				for (int i = 0; i < current_map.pl_nr;i++) {
					if (players[i].connected) connected_d++;
				}
				if (connected_d != connected_count) {
					connected_count = connected_d;
					lobby_count count;
					count.count = connected_count;
					ENetPacket* count_pack = enet_packet_create(&count, sizeof(lobby_count), ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, count_pack);
				}
				if ((connected_count == 6)||(connect_anyway)) {
					current_state = ServerState::MATCH_IN_PROGRESS;

					MatchStatePacket state_pkt{ .new_state = static_cast<uint8_t>(ServerState::MATCH_IN_PROGRESS) };
					ENetPacket* packet = enet_packet_create(&state_pkt, sizeof(MatchStatePacket), ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, packet);
					cout << "Match automatically started with " << connected_count << " players!\n";
				}
			}
			else if (current_state == ServerState::MATCH_IN_PROGRESS) {
				int connections=0;
				for (int i = 0; i < current_map.pl_nr; i++) {
					if (players[i].connected) connections++;
				}
				connected_count = connections;
				if (connections == 0) {
					current_state = ServerState::GAME_OVER;
					break;
				}
				simulation_pl_pr(current_map,players, projectiles, tick_time, projectile_ids);
				int alive_count = 0;
				int last_alive_id = -1;
				for (int i = 0; i < current_map.pl_nr; i++) {
					if (players[i].active&& players[i].health > 0) {
						alive_count++;
						last_alive_id = players[i].id;
					}
				}
				for (int i = 0; i < current_map.pl_nr; i++) {
					if (players[i].health <= 0) {
						if (players[i].connected) {
							cout << "Sending packet to Player " << players[i].id << " with rank " << (alive_count + 1) << std::endl;
							MatchStatePacket win_pkt{
								.new_state = static_cast<uint8_t>(ServerState::GAME_OVER),
								.position = static_cast<uint8_t>(alive_count + 1),
								.trophies = static_cast<int8_t>(trophy_payout[alive_count]),
								.coins = static_cast<uint8_t>(coin_payout[alive_count])
							};
							ENetPacket* packet = enet_packet_create(&win_pkt, sizeof(MatchStatePacket), ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(players[i].peer, 0, packet);
							players[i].connected = false;
						}
						continue;
					}
				}
				if (alive_count <= 1) {
					current_state = ServerState::GAME_OVER;
					MatchStatePacket win_pkt{
								.new_state = static_cast<uint8_t>(ServerState::GAME_OVER),
								.position = 1,
								.trophies = static_cast<int8_t>(trophy_payout[0]),
								.coins = static_cast<uint8_t>(coin_payout[0])
					};
					ENetPacket* packet = enet_packet_create(&win_pkt, sizeof(MatchStatePacket), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(players[last_alive_id].peer, 0, packet);
					cout << "Game Over! Player " << last_alive_id << " wins the match.\n";
				}

				network_pl_pr(current_map, players, projectiles, max_view_x, max_view_y) ;
			}
			else if (current_state == ServerState::GAME_OVER) {
				cout << "Resetting server playground back to Lobby state...\n";
				projectiles.clear();

				for (int i = 0; i < current_map.pl_pos.size(); i++) {
					player pl_temp{};
					pl_temp.id = i;
					pl_temp.pos_x = current_map.pl_pos[i][1];
					pl_temp.pos_y = current_map.pl_pos[i][0];
					pl_temp.shape = shapes[0];
					pl_temp.shape_level = 0;
					pl_temp.ammo_now = pl_temp.shape.ammo;
					pl_temp.health = pl_temp.shape.levels[0].health;
					pl_temp.shape.ammo_damage = pl_temp.shape.levels[0].ammo_damage;
					pl_temp.active = true;
					players[i] = pl_temp;
				}
				connected_count = 0;
				current_state = ServerState::LOBBY;
				break;
			}
			accumulated_time -= tick_time;
		}
		double time_left = tick_time - accumulated_time;
		if (time_left > 0.005) {
			long long sleep_us = static_cast<long long>((time_left - 0.002) * 1000000);

			if (sleep_us > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
			}
		}
	}
	enet_host_destroy(server);
	return 0;
}
	