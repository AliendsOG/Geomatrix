#include "CMakeProject1.h"
#include "enet/enet.h"

using std::cout;
using std::string;
//network stuff begin
#pragma pack(push, 1)
#define MAX_PLAYERS 10
#define MAX_PROJECTILES 100
enum class PacketType : uint8_t {
	ID_ASSIGNMENT = 0,
	WORLD_STATE = 1
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
	PlayerNetworkState players[MAX_PLAYERS];
	projectile_network projectiles[MAX_PROJECTILES];

};
struct id_assign {
	uint8_t packet_type = static_cast<uint8_t>(PacketType::ID_ASSIGNMENT); 
	uint8_t id;
};
#pragma pack(pop)

//network stuff end

struct map {
	int height{};
	int width{};
	int scale{};
	int pl_nr{};
	std::vector<std::vector<int>> matrix;
	std::vector<std::vector<int>> pl_pos;
	map(int h, int w, int s, int n) :
		height(h),
		width(w),
		scale(s),
		pl_nr(n),
		pl_pos(n,std::vector<int>(2))
	{}
};
const int pl_width = 96;
struct player {
	int id{};
	int health{};
	string name;
	int ammo{};
	float reload_time{};
	long long reload_time_start{};
	long long reload_time_now{};
	int range{};
	int ammo_r{};
	int ammo_speed{};
	int ammo_damage{};
	int pos_x{};
	int pos_y{};
	int new_x{};
	int new_y{};
	float pos_f_x{};
	float pos_f_y{};
	float joy_x{};
	float joy_y{};
	int speed{};
	float aim_x{};
	float aim_y{};
	bool attack = false;
	bool active = true;
	bool connected = false;
	ENetPeer* peer{ nullptr };
	player(int h, string name, int x, int y, int range, int s, int a_r, int a_speed,int id, int a_d) :
		health(h),
		id(id),
		name(name),
		pos_x(x),
		pos_y(y),
		new_x(x),
		new_y(y),
		pos_f_x(static_cast<int>(x)),
		pos_f_y(static_cast<int>(y)),
		range(range),
		speed(s),
		ammo_r(a_r),
		ammo_speed(a_speed),
		ammo_damage(a_d)
	{}
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

struct player_input {
	float x{};
	float y{};
	bool attack{};
	float aim_x{};
	float aim_y{};
};

bool collision(map& map, int& new_x, int& new_y) {
	int corners_x[4] = { new_x - pl_width / 2, new_x + pl_width / 2 - 1, new_x - pl_width / 2, new_x + pl_width / 2 - 1 };
	int corners_y[4] = { new_y - pl_width / 2, new_y - pl_width / 2, new_y + pl_width / 2 - 1, new_y + pl_width / 2 - 1 };

	for (int i = 0; i < 4; i++) {
		int tile_x = corners_x[i] / map.scale;
		int tile_y = corners_y[i] / map.scale;

		if (tile_x < 0 || tile_x >= map.width || tile_y < 0 || tile_y >= map.height) return false;

		int tile_type = map.matrix[tile_y][tile_x];
		// 0 = floor, 2 = bush, everything else (like 3 or 4) is solid wall
		if (tile_type != 0 && tile_type != 2) {
			return false;
		}
	}
	return true;
}
bool aim(map& map, std::vector<player>& players, projectile& pr, float d_time) {
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
	//int* tile = &map.coordonates[pr.new_y][pr.new_x];
	int* tile = &map.matrix[pr.new_y/map.scale][pr.new_x/map.scale];

	if ((*tile == 3) || (*tile == 4)) {
		return false;
	}
	pr.distance += std::sqrt(pow(pr.new_x - pr.pos_x, 2) + pow(pr.new_y - pr.pos_y, 2));
	pr.pos_x = pr.new_x;
	pr.pos_y = pr.new_y;
	for (int i = 0; i < players.size(); i++) {
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
	pl.pos_f_x += pl.joy_x * pl.speed * d_time;
	pl.pos_f_y += pl.joy_y * pl.speed * d_time;
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



int main() {
	bool active = true;
	std::vector<player> players;
	std::vector<projectile> projectiles;
	map default_map(39, 39,128,6);
	int max_view_x = default_map.width * default_map.scale / 2;
	int max_view_y = default_map.height * default_map.scale / 2/16*9;
	default_map.matrix = {
		{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}, // 1
		{4,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,4}, // 2
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 3
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 4
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 5
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 6
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 7
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 8
		{4,2,2,2,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,2,2,2,4}, // 9
		{4,0,0,0,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,0,0,0,4}, // 10
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 11
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 12
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 13
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 14
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 15
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 16
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 17
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 18
		{4,0,0,4,4,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,4,4,0,0,4}, // 19
		{4,0,0,4,4,0,0,0,0,0,0,0,0,0,0,3,3,0,0,4,0,0,3,3,0,0,0,0,0,0,0,0,0,0,4,4,0,0,4}, // 20 
		{4,0,0,4,4,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,4,4,0,0,4}, // 21
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 22
		{4,0,0,0,0,0,3,3,3,3,0,0,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,0,0,3,3,3,3,0,0,0,0,0,4}, // 23
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 24
		{4,0,3,3,0,0,4,4,4,4,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,0,0,0,0,4,4,4,4,0,0,3,3,0,4}, // 25
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 26
		{4,0,4,4,0,0,0,0,0,2,2,2,2,0,1,1,1,1,0,0,0,1,1,1,1,0,2,2,2,2,0,0,0,0,0,4,4,0,4}, // 27
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 28
		{4,0,0,0,0,3,3,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,3,3,0,0,0,0,4}, // 29
		{4,0,0,0,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,0,0,0,4}, // 30
		{4,2,2,2,0,4,4,0,0,0,0,0,0,0,3,3,4,4,0,0,0,4,4,3,3,0,0,0,0,0,0,0,4,4,0,2,2,2,4}, // 31
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 32
		{4,2,2,2,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,2,2,2,4}, // 33
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 34
		{4,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,4}, // 35
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 36
		{4,0,0,4,4,3,3,4,4,0,0,0,0,2,2,2,0,0,4,4,4,0,0,0,2,2,2,0,0,0,0,4,4,3,3,4,4,0,4}, // 37
		{4,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,4}, // 38
		{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}  // 39
	};
	default_map.pl_pos = {
		{ 4 * default_map.scale + default_map.scale/2, 17 * default_map.scale + default_map.scale/2},
		{ 4 * default_map.scale + default_map.scale/2, 19 * default_map.scale + default_map.scale/2},
		{ 4 * default_map.scale + default_map.scale/2, 21 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 17 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 19 * default_map.scale + default_map.scale/2},
		{34 * default_map.scale + default_map.scale/2, 21 * default_map.scale + default_map.scale/2},
	};
	for (int i = 0; i < default_map.pl_pos.size(); i++) {
		player pl_temp(1000, "Player" + std::to_string(i), default_map.pl_pos[i][1], default_map.pl_pos[i][0], 960,640, 26,960,i, 100);
		pl_temp.reload_time = 1;	
		pl_temp.ammo = 3;
		pl_temp.active = true;
		pl_temp.peer = nullptr;
		players.push_back(pl_temp);
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
	while (active) {
		while (enet_host_service(server, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				cout << "A new client connected from "
					<< (*event.peer).address.host << ":"
					<< event.peer->address.port << "\n";
				
				for (int i = 0; i < players.size(); i++) {
					if (!players[i].connected) {
						players[i].connected = true;
						players[i].peer = event.peer; 
						players[i].id = i;
						event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(i));
						id_assign id_sent;
						id_sent.id = i;
						ENetPacket* packet = enet_packet_create(&id_sent, sizeof(id_assign), 0);
						enet_peer_send(event.peer, 0, packet);
						break;
					}
				}
				
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
				if (event.packet->dataLength == sizeof(player_input)) {
					player_input* received_input = (player_input*)event.packet->data;
					uintptr_t target_id = (uintptr_t)event.peer->data;
					for (auto& pl : players) {
						if (pl.id == target_id) {
							pl.joy_x = received_input->x;
							pl.joy_y = received_input->y;
							pl.attack = received_input->attack;
							if (received_input->attack) {
								pl.aim_x =received_input->aim_x;
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
					std::cerr << "Received a malformed packet of unexpected size.\n";
				}
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT: {
				if (event.peer->data != nullptr) {
					uintptr_t disconnected_player = reinterpret_cast<uintptr_t>(event.peer->data);
					cout << "Player " << disconnected_player << " disconnected." << std::endl;
					for (auto it = players.begin(); it != players.end(); ++it) {
						if (it->peer == players[disconnected_player].peer) {
							players.erase(it);
							break; 
						}
					}
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
			for (int i = 0; i < players.size(); i++) {
				if ((players[i].connected)&& (players[i].active)) {
					movement_float(default_map, players[i], tick_time);
					if ((players[i].attack) && (projectiles.size() < MAX_PROJECTILES)) {
						projectile pr(players[i].ammo_r, players[i].ammo_speed, players[i].range, players[i].ammo_damage);
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
					}
				}
			}
			auto it = projectiles.begin();
			while (it != projectiles.end()) {
				bool is_alive = aim(default_map, players, *it, tick_time);

				if (!is_alive) {
					it = projectiles.erase(it);
				}
				else {
					++it; 
				}
			}
			for (int i = 0; i < players.size(); i++) {
				if ((players[i].connected) && (players[i].active)) {
					if (players[i].health <= 0) {
						players[i].active = false;
						continue;
					}

					WorldStatePacket client_packet = { 0 };
					client_packet.packet_type = static_cast<uint8_t>(PacketType::WORLD_STATE);
					client_packet.active_player_count = 0;
					player* pl = &players[i];
					for (int j = 0; j < players.size(); j++) {
						if ((players[j].id != pl->id) && (players[j].active)) {
							int distance_x = abs(players[j].pos_x - pl->pos_x);
							int distance_y = abs(players[j].pos_y - pl->pos_y);
							int view_y = max_view_y;
							if (pl->pos_y - max_view_y < 0) {
								view_y -= pl->pos_y - max_view_y;
							}else if (pl->pos_y + max_view_y > default_map.height * default_map.scale) {
								view_y += pl->pos_y + max_view_y - default_map.height * default_map.scale;
							}
							int view_x = max_view_x;
							if (pl->pos_x - max_view_x < 0) {
								view_x -= pl->pos_x - max_view_x;
							}else if (pl->pos_x + max_view_x > default_map.width*default_map.scale) {
								view_x += pl->pos_x + max_view_x- default_map.width * default_map.scale;
							}
							if ((distance_x <= view_x) && (distance_y <= view_y)) {
								client_packet.players[client_packet.active_player_count].pos_x = static_cast<uint16_t>(players[j].pos_x);
								client_packet.players[client_packet.active_player_count].pos_y = static_cast<uint16_t>(players[j].pos_y);
								client_packet.players[client_packet.active_player_count].active = players[j].active;
								client_packet.players[client_packet.active_player_count].health = static_cast<uint16_t>(players[j].health);
								client_packet.players[client_packet.active_player_count].id = static_cast<uint16_t>(players[j].id);
								client_packet.active_player_count++;
							}
						}
						else if (players[j].id == pl->id) {
							client_packet.players[client_packet.active_player_count].pos_x = static_cast<uint16_t>(players[j].pos_x);
							client_packet.players[client_packet.active_player_count].pos_y = static_cast<uint16_t>(players[j].pos_y);
							client_packet.players[client_packet.active_player_count].active = players[j].active;
							client_packet.players[client_packet.active_player_count].id = static_cast<uint16_t>(players[j].id);
							client_packet.players[client_packet.active_player_count].health = static_cast<uint16_t>(players[j].health);
							client_packet.active_player_count++;
						}
					}
					client_packet.projectile_count = 0;
					auto it = projectiles.begin();
					while (it != projectiles.end()) {
						int distance_x = abs(it->pos_x - pl->pos_x);
						int distance_y = abs(it->pos_y - pl->pos_y);
						if ((distance_x <= max_view_x) && (distance_y <= max_view_y)) {
							client_packet.projectiles[client_packet.projectile_count].pos_x = static_cast<uint16_t>(it->pos_x);
							client_packet.projectiles[client_packet.projectile_count].pos_y = static_cast<uint16_t> (it->pos_y);
							client_packet.projectiles[client_packet.projectile_count].radius = static_cast<uint16_t>(it->radius);
							client_packet.projectiles[client_packet.projectile_count].id = static_cast<uint16_t>(it->pr_id);
							client_packet.projectile_count++;
						}
						++it; 
					}			
					int dynamic_size = sizeof(uint8_t) * 3 
						+ (client_packet.active_player_count * sizeof(PlayerNetworkState))
						+ (client_packet.projectile_count * sizeof(projectile_network));
					ENetPacket* packet= enet_packet_create(&client_packet, dynamic_size, 0);
					enet_peer_send(players[i].peer, 0, packet);
				}
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
	