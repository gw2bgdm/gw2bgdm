#include "server/context.h"
#include "server/lru_cache.h"
#include "server/debug.h"
#include "server/network.h"
#include "server/crypto.h"
#include "core/logging.h"
#include "core/crc32.h"
#include "core/file.h"
#include "core/message.h"
#if defined(_WIN32) || defined (_WIN64)
#include "core/time.h"
#else
#include "core/time_.h"
#endif
#include "core/types.h"
#include "core/helpers.h"
#include "sqlite3/sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

#if defined(_WIN32) || defined (_WIN64)
#include "meter/utf.h"
#include <direct.h>
#include <windows.h>
#include <strsafe.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#define MAX_PATH 260
#endif


//#if !defined(__APPLE__) && !defined(_WIN32) && !defined (_WIN64)
//struct tm *gmtime_r(const time_t * __restrict, struct tm * __restrict);
//struct tm *localtime_r(const time_t * __restrict, struct tm * __restrict);
//#endif

// Check frequency for a new meter file.
#define FREQ_METER_CHECK 10000000

// Path to the meter resources.
#define PATH_METER_DLL "bgdm.dll"
#define PATH_SIG_EXT ".sig"
#define PATH_METER_LOG "bgdm.server.log"
#define PATH_METER_DB "bgdm.server.db"

// Max server send piece retries
#define MSG_MAX_RETRIES 10

// Update data.
static u8* g_update_dll;
static u8* g_update_sig;
static u32 g_update_dll_size;
static u32 g_update_sig_size;
static u32 g_update_version;
static i64 g_update_time;
static i8 g_dll_path[MAX_PATH] = { 0 };

logger_t s_logger;
logger_filter_t  s_filter_scr;
logger_filter_t  s_filter_file;
logger_formatter_t s_formatter;
logger_handler_t s_handler_file;
logger_handler_t s_handler_scr;

sqlite3 *s_db = 0;

// Load the update DLL
static bool meter_load()
{

#if defined(_WIN32) || defined (_WIN64)

	static volatile LONG m_lock = 0;

	// EnterSpinLcok
	while (InterlockedCompareExchange(&m_lock, 1, 0))
	{
	}
#else
	static pthread_mutex_t m_lock;
	pthread_mutex_lock(&m_lock);
#endif

	if (!g_dll_path[0])
		return false;

	// Release old data.
	if (g_update_dll)
		free(g_update_dll);
	g_update_dll = 0;
	g_update_sig = 0;

	// Read the file and the sig.
	g_update_dll = file_read(g_dll_path, &g_update_dll_size);
	if (g_update_dll == 0)
	{
		LOG_ERR("[server] couldn't find update dll at %s", g_dll_path);
		return false;
	}

#if defined(_WIN32) || defined (_WIN64)
	// Dynamically generate the signature data
	if (!crypto_sign_bytes(g_update_dll, g_update_dll_size, &g_update_sig, &g_update_sig_size, g_dll_path))
	{
		LOG_ERR("[server] unable to generate update file signature");
		free(g_update_dll);
		g_update_dll = 0;
		return false;
	}
#else
	i8 sig_path[MAX_PATH] = { 0 };
	size_t len = strlen(g_dll_path);
	strncpy(sig_path, g_dll_path, ARRAYSIZE(sig_path));
	if (len>4) sprintf(&sig_path[len - 4], PATH_SIG_EXT);
	g_update_sig = file_read(sig_path, &g_update_sig_size);
	if (g_update_sig == 0)
	{
		LOG_ERR("[server] couldn't find signature file at %s", sig_path);
		return false;
	}
#endif

	// Get the update version.
	g_update_version = crc32(g_update_dll, g_update_dll_size);

	// Save last modification time of the DLL
	g_update_time = file_get_time(g_dll_path);

	// Display the update.
	LOG_INFO("[server] local file version %x, size %d", g_update_version, g_update_dll_size);

#if defined(_WIN32) || defined (_WIN64)

	// ExitSpinLcok
	InterlockedExchange(&m_lock, 0);
#else
	pthread_mutex_unlock(&m_lock);
#endif

	return true;
}

// Returns true if the meter files have changed.
static bool meter_has_changed(i64 now)
{
	static i64 last_check;

	if (!g_dll_path[0]) {
		// error, meter not initialized yet
		return false;
	}

	if (now - last_check < FREQ_METER_CHECK)
	{
		return false;
	}

	last_check = now;

	i64 file_time = file_get_time(g_dll_path);
	if (file_time == g_update_time)
	{
		return false;
	}

	g_update_time = file_time;

	return true;
}

// Handles a client damage message.
void msg_client_damage_utf8(Context* ctx, MsgClientDamageUTF8* msg, u32 addr, u32 port, i64 now)
{
	i8 str_addr[22] = { 0 };
	network_addr_to_str(addr, str_addr);

	// Verify the message.
	if (msg->shard_id == 0)
	{
		return;
	}

	// Find or insert the user into the table.
	User* user = context_insert_user(ctx, addr, port, now);
	if (user == 0)
	{
		return;
	}

	// Disgard messages that occur too frequently.
	if (now - user->last_msg_damage < MSG_FREQ_CLIENT_DAMAGE)
	{
		LOG(12, "[client:%s:%d] client is sending damage updates too often.", str_addr, port);
		return;
	}

	// Update the user data.
	user->last_msg_damage = now;
	user->profession = msg->profession;
	user->in_combat = msg->in_combat;
	user->time = msg->time;
	user->damage = msg->damage;
	user->damage_in = msg->damage_in;
	user->heal_out = msg->heal_out;
	user->stats = msg->stats;
	user->shard_id = msg->shard_id;
	context_update_name(ctx, user, msg->name);
	memcpy(user->targets, msg->targets, sizeof(user->targets));

	char str_dmgOut[32] = { 0 };
	char str_dmgIn[32] = { 0 };
	char str_healOut[32] = { 0 };
	sprintf_num(msg->damage, str_dmgOut, sizeof(str_dmgOut));
	sprintf_num(msg->damage_in, str_dmgIn, sizeof(str_dmgIn));
	sprintf_num(msg->heal_out, str_healOut, sizeof(str_healOut));
	LOG(13, "[client:%s:%d] [%s] [dout=%s] [din=%s] [heal=%s] [time=%.2f]",
		str_addr, port, user->utf8_name, str_dmgOut, str_dmgIn, str_healOut, msg->time / 1000.0f);

	// Prepare the response message.
	MsgServerDamageUTF8 res = { 0 };
	res.msg_type = MSG_TYPE_SERVER_DAMAGE_UTF8;
	res.selected_id = msg->selected_id;

	u32 num = 0;
	for (u32 i = 0; i < MSG_CLOSEST_PLAYERS; ++i)
	{
		if (msg->closest[i].name[0] == 0)
			break;

		LOG(5, "[client:%s:%d] [%s] closest(%d)=%s", str_addr, port, user->utf8_name, i, msg->closest[i].name);

		// Find the user by name.
		User* u = context_find_user(ctx, msg->closest[i].name, user->shard_id);
		if (u == 0)
		{
			continue;
		}

		// Add the user to the closest array.
		res.closest[num].in_combat = u->in_combat;
		res.closest[num].time = u->time;
		res.closest[num].damage = u->damage;
		res.closest[num].damage_in = u->damage_in;
		res.closest[num].heal_out = u->heal_out;
		res.closest[num].stats = u->stats;
		res.closest[num].profession = u->profession;
		memcpy(res.closest[num].name, u->utf8_name, sizeof(res.closest[num].name));
		memcpy(res.closest[num].targets, u->targets, sizeof(res.closest[num].targets));
		++num;
	}

	// Check if any users matched.
	if (num == 0)
	{
		return;
	}

	LOG(14, "[server] sending %d closest to [client:%s:%d] [%s]", num, str_addr, port, user->utf8_name);

	// Send the message.
	network_send(&res, sizeof(res), addr, port);
}

static int db_update_user(User* user, u32 version, u64 sequence, u32 addr, u32 port, i64 now)
{
	i8 str_addr[22] = { 0 };
	network_addr_to_str(addr, str_addr);

	if (!s_db || !user)
		return SQLITE_ERROR;

	struct tm tm = { 0 };
	time_t curtime = 0;
	i8 nowstr[32];
	i8 verstr[32];
	int rc = 0;
	int id = 0;
	i8 *sql = 0;
	i8 *sqlIP = "SELECT * FROM [Users] WHERE IpAddress = ?";
	i8 *sqlAcct = "SELECT * FROM [Users] WHERE AcctName = ?";
	sqlite3_stmt *stmt = 0;
	bool hasAcct = (user->utf8_acctName[0] != 0 || strcmp(user->utf8_acctName, "") != 0);
	bool hasName = (user->utf8_name[0] != 0 || strcmp(user->utf8_name, "") != 0);
	bool useIP = !hasAcct;

	if (!hasAcct)
		// 2016-12-29: only have 2 users left with the old version
		// skip any request without acctName
		return SQLITE_ERROR;

	if (user->ver_str[0]) strncpy(verstr, user->ver_str, ARRAYSIZE(verstr));
	else sprintf(verstr, "%x", version);

	time(&curtime);
#if defined(_WIN32) || defined(_WIN64)
	localtime_s(&tm, &curtime);
#else
	localtime_r(&curtime, &tm);
#endif


	strftime(&nowstr[0], sizeof(nowstr), "%Y-%m-%dT%H:%M:%SZ", &tm);

	rc = sqlite3_prepare_v2(s_db, useIP ? sqlIP : sqlAcct, -1, &stmt, 0);
	if (rc != SQLITE_OK)
	{
		LOG_WARN("[server] Database search failed for %s, error: %s",
			/*useIP ? str_addr : */user->utf8_acctName, sqlite3_errmsg(s_db));
		sqlite3_finalize(stmt);
		return rc;
	}

	sqlite3_bind_text(stmt, 1, /*useIP ? str_addr : */user->utf8_acctName, -1, SQLITE_STATIC);
	rc = sqlite3_step(stmt);

	if (!useIP)
	{
		if (rc == SQLITE_ROW)
		{
			// Account name found, get the user ID
			id = sqlite3_column_int(stmt, 0);
		}
		else
		{
			// Unable to find acctName search again using IP
			sqlite3_finalize(stmt);

			useIP = true;
			rc = sqlite3_prepare_v2(s_db, sqlIP, -1, &stmt, 0);
			sqlite3_bind_text(stmt, 1, str_addr, -1, SQLITE_STATIC);
			rc = sqlite3_step(stmt);
		}
	}

	if (useIP && rc == SQLITE_ROW) {

		do
		{
			// IP found, only update the row if acctName IS NULL
			// Otherwise we might be overwriting a valid acctName
			const u8* acctName = sqlite3_column_text(stmt, 1);
			if (!acctName || acctName[0] == 0 || strcmp((const i8*)acctName, "") == 0)
			{
				id = sqlite3_column_int(stmt, 0);
				break;
			}

			rc = sqlite3_step(stmt);
		} while (rc == SQLITE_ROW);

		// If the IP is already in the DB and has valid acctName
		// we do not insert a new empty line
		if (!hasAcct && id == 0)
		{
			sqlite3_finalize(stmt);
			return SQLITE_ERROR;
		}
	}
	sqlite3_finalize(stmt);

	if (id != 0)
	{
		if (hasName)
		{
			// Update exisiting user
			sql = "UPDATE [Users] SET "
				"AcctName = ?, "
				"IpAddress = ?, "
				"Version = ?, "
				"VersionStr = ?, "
				"LastSeen = ?, "
				"CharName = ? "
				"WHERE ID = ?;";
		}
		else
		{
			sql = "UPDATE [Users] SET "
				"AcctName = ?, "
				"IpAddress = ?, "
				"Version = ?, "
				"VersionStr = ?, "
				"LastSeen = ? "
				"WHERE ID = ?;";
		}
	}
	else
	{
		// Inser new user
		if (hasName)
			sql = "INSERT INTO [Users](AcctName, IpAddress, Version, VersionStr, LastSeen, CharName, FirstSeen) VALUES (?, ?, ?, ?, ?, ?, ?);";
		else
			sql = "INSERT INTO [Users](AcctName, IpAddress, Version, VersionStr, LastSeen, FirstSeen) VALUES (?, ?, ?, ?, ?, ?);";
	}

	rc = sqlite3_prepare_v2(s_db, sql, -1, &stmt, 0);
	if (rc == SQLITE_OK)
	{
		int i = 0;

		sqlite3_bind_text(stmt, ++i, hasAcct ? user->utf8_acctName : NULL, -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, ++i, str_addr, -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, ++i, version);
		sqlite3_bind_text(stmt, ++i, verstr, -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, ++i, nowstr, -1, SQLITE_STATIC);

		if (hasName)
			sqlite3_bind_text(stmt, ++i, hasName ? user->utf8_name : NULL, -1, SQLITE_STATIC);

		if (id == 0)
			sqlite3_bind_text(stmt, ++i, nowstr, -1, SQLITE_STATIC);
		else
			sqlite3_bind_int(stmt, ++i, id);

		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			LOG_WARN("[server] Database failed to %s user %d:%s (IP:%s), error: %s",
				(id != 0) ? "UPDATE" : "INSERT",
				id, user->utf8_acctName, str_addr,
				sqlite3_errmsg(s_db));
		}
		else
		{
			LOG(8, "[server] Database successfully %sD user %d:%s (IP:%s)",
				(id != 0) ? "UPDATE" : "INSERTE",
				id, user->utf8_acctName, str_addr);
		}
		sqlite3_finalize(stmt);
	}

	return rc;
}

// Handles a client update message.
void msg_client_update_begin(User* user, u32 version, u64 sequence, u32 addr, u32 port, i64 now)
{
	i8 str_addr[22] = { 0 };
	network_addr_to_str(addr, str_addr);

	// Check if the client is asking to update too often.
	if (user->last_msg_update_begin != 0 && now - user->last_msg_update_begin < MSG_FREQ_CLIENT_UPDATE_BEGIN)
	{
		LOG(12, "[client:%s:%d] %s (%s) is asking updates too often.", str_addr, port, user->utf8_charName, user->utf8_acctName);
		return;
	}

	// Update the user.
	user->msg_update_retry = 0;
	user->last_msg_update_seq = sequence;
	user->last_msg_update_begin = now;

	// Check if the file data has changed.
	if (g_update_dll == 0 || meter_has_changed(now))
	{
		if (!meter_load())
			return;
	}

	// If the files aren't different, ignore them.
	if (version == g_update_version)
	{
		LOG_INFO("[client:%s:%d] %s (%s) has the latest version %s (%x).", str_addr, port, user->utf8_charName, user->utf8_acctName, user->ver_str, g_update_version);
		// we are basically using the begin update check as a ping now
		// so the client always knows it has the most updated version
		//return;
	}
	else
	{
		LOG_INFO("[client:%s:%d] %s (%s) has version %x, initiating update to version %x", str_addr, port, user->utf8_charName, user->utf8_acctName, version, g_update_version);
	}

	// Save the user in the localDB
	db_update_user(user, version, sequence, addr, port, now);

	// Start the update.
	MsgServerUpdateBegin res = { 0 };
	res.msg_type = MSG_TYPE_SERVER_UPDATE_BEGIN;
	res.sequence = sequence;
	res.data_version = g_update_version;
	res.data_size = g_update_dll_size;
	memcpy(res.sig, g_update_sig, sizeof(res.sig) < g_update_sig_size ? sizeof(res.sig) : g_update_sig_size);
	res.pubkey_len = crypto_pubkey_bytes();
	memcpy(res.pubkey_data, crypto_pubkey_data(), res.pubkey_len);

	network_send(&res, sizeof(res), addr, port);
}

void msg_client_update_begin_utf8(Context* ctx, MsgClientUpdateBeginUTF8* msg, u32 addr, u32 port, i64 now)
{
	i8 str_addr[22] = { 0 };
	network_addr_to_str(addr, str_addr);

	// Verify the message.
	if (msg->data_version == 0)
	{
		return;
	}

	// Find or insert the user into the table.
	User* user = context_insert_user(ctx, addr, port, now);
	if (user == 0)
	{
		return;
	}

	strncpy(user->ver_str, msg->version, ARRAYSIZE(user->ver_str));

	context_update_account(ctx, user, msg->acctName, msg->charName);

	msg_client_update_begin(user, msg->data_version, msg->sequence, addr, port, now);
}


// Handles a client update message.
void msg_client_update_piece(Context* ctx, MsgClientUpdatePiece* msg, u32 addr, u32 port, i64 now)
{
	i8 str_addr[22] = { 0 };
	network_addr_to_str(addr, str_addr);

	if (msg->data_size < 1 || msg->data_size > MSG_UPDATE_SIZE || msg->data_version != g_update_version || msg->data_offset + msg->data_size > g_update_dll_size)
	{
		return;
	}

	// Find or insert the user into the table.
	User* user = context_insert_user(ctx, addr, port, now);
	if (user == 0)
	{
		return;
	}

	// Check if we're updating too fast.
	if (user->last_msg_update_piece != 0 && now - user->last_msg_update_piece < MSG_FREQ_CLIENT_UPDATE_PIECE)
	{
		return;
	}

	if (user->msg_update_max_size == 0)
		user->msg_update_max_size = msg->data_size;

	if (user->last_msg_update_seq == msg->sequence)
		user->msg_update_retry++;
	else
		user->msg_update_retry = 0;
	user->last_msg_update_seq = msg->sequence;

	if (user->msg_update_retry > MSG_MAX_RETRIES)
	{
		bool reset = true;
		u32 size = msg->data_size;
		while (size > 1023 && user->msg_update_max_size > 1023)
		{
			// Min packet size we allow is 512 (1024/2)
			if (user->msg_update_max_size == size)
			{
				// Try with smaller packet size
				// before reverting to a reset
				user->msg_update_max_size = size / 2;
				user->msg_update_retry = 0;
				reset = false;

				LOG_WARN("[client:%s:%d] [seq=%x] TOO MANY RETRIES @ OFFSET %.1f/%.1fKB, REDUCING PACKET SIZE TO %d bytes",
					str_addr,
					port,
					msg->sequence,
					msg->data_offset / 1024.0f,
					g_update_dll_size / 1024.0f,
					user->msg_update_max_size
				);
				break;
			}
			size = size / 2;
		}

		if (reset)
		{
			// Too many retries, send a reset to the client
			user->msg_update_retry = 0;
			user->msg_update_max_size = 0;

			LOG_WARN("[client:%s:%d] [seq=%x] TOO MANY RETRIES @ OFFSET %.1f/%.1fKB, SENDING RESET",
				str_addr,
				port,
				msg->sequence,
				msg->data_offset / 1024.0f,
				g_update_dll_size / 1024.0f
			);

			/*MsgServerUpdateBegin res = { 0 };
			res.msg_type = MSG_TYPE_SERVER_UPDATE_BEGIN;
			res.sequence = msg->sequence;
			res.data_version = g_update_version;
			res.data_size = msg->data_offset > 0 ? msg->data_offset : g_update_dll_size;
			memcpy(res.sig, g_update_sig, sizeof(res.sig) < g_update_sig_size ? sizeof(res.sig) : g_update_sig_size);
			res.pubkey_len = crypto_pubkey_bytes();
			memcpy(res.pubkey_data, crypto_pubkey_data(), res.pubkey_len);

			network_send(&res, sizeof(res), addr, port);*/
			return;
		}
	}

	MsgServerUpdatePiece res = { 0 };
	MsgServerUpdatePieceNoFrag resNoFrag = { 0 };
	MsgServerUpdatePiece *pMsg = &res;
	u32 msgSize = sizeof(MsgServerUpdatePiece);

	if (user->msg_update_max_size < 1023) {
		// Instead of sending the 512KB packet
		// reduce it to the no fragmented UDP packet
		pMsg = (MsgServerUpdatePiece*)&resNoFrag;
		msgSize = sizeof(MsgServerUpdatePieceNoFrag);
		user->msg_update_max_size = MSG_UPDATE_SIZE_NO_FRAG;
		LOG_DEBUG("[client:%s:%d] [ver=%x] [seq=%x] REDUCED PACKET SIZE TO MAX_UDP_NO_FRAG (%d bytes)",
			str_addr,
			port,
			msg->data_version,
			msg->sequence,
			msgSize
		);
	}

	// Copy the data into the message.
	pMsg->msg_type = MSG_TYPE_SERVER_UPDATE_PIECE;
	pMsg->sequence = msg->sequence;
	pMsg->data_version = msg->data_version;
	pMsg->data_offset = msg->data_offset;
	pMsg->data_size = min(user->msg_update_max_size, msg->data_size);
	memcpy(pMsg->data_buffer, g_update_dll + msg->data_offset, pMsg->data_size);

	network_send(pMsg, msgSize, addr, port);

	LOG_INFO("[client:%s:%d] [ver=%x] [seq=%x] SENT %d bytes @ OFFSET %.1f/%.1fKB",
		str_addr,
		port,
		pMsg->data_version,
		pMsg->sequence,
		pMsg->data_size,
		pMsg->data_offset / 1024.0f,
		g_update_dll_size / 1024.0f
	);
}

static int db_create(i8* cwd)
{
	static i8 dbpath[MAX_PATH];

	//int rc = sqlite3_open(":memory:", &s_db);
#if defined(_WIN32) || defined (_WIN64)
	sprintf(dbpath, "%s\\%s", cwd, PATH_METER_DB);
#else
	sprintf(dbpath, "%s/%s", cwd, PATH_METER_DB);
#endif
	int rc = sqlite3_open(dbpath, &s_db);
	if (rc != SQLITE_OK) {
		LOG_WARN("[server] Unable to open database, error: %s", sqlite3_errmsg(s_db));
		s_db = 0;
		return rc;
	}

	int ret = SQLITE_ERROR;
	sqlite3_stmt *stmt = 0;
	rc = sqlite3_prepare_v2(s_db, "SELECT SQLITE_VERSION()", -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		LOG_WARN("[server] Unable to get database version, error: %s", sqlite3_errmsg(s_db));
		sqlite3_finalize(stmt);
		sqlite3_close(s_db);
		s_db = 0;
		ret = rc;
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
			LOG_INFO("[server] SQLite DB initialzed, version: %s", sqlite3_column_text(stmt, 0));
		}
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(s_db, "SELECT * FROM [Users] LIMIT 1", -1, &stmt, 0);
		if (rc != SQLITE_OK) {
			LOG_WARN("[server] Failed to verify database, error: %s", sqlite3_errmsg(s_db));
			sqlite3_close(s_db);
			s_db = 0;
		}
		else
		{
			ret = SQLITE_OK;
		}
		sqlite3_finalize(stmt);
	}
	return ret;
}

static bool logger_create(i8* cwd, i32 loggingMinLevel)
{
	static i8 logpath[MAX_PATH];
	FILE *logfile;

	memset(&s_logger, 0, sizeof(s_logger));
	s_logger.name = "bgdm-logger";

	if (loggingMinLevel == -1)
	{
#if defined(_DEBUG) || defined(DEBUG) || defined(DBG)
		loggingMinLevel = 0;
#else
		loggingMinLevel = 16;
#endif
	}

	s_filter_scr.minlevel = loggingMinLevel;
	s_filter_scr.maxlevel = LOGGING_MAX_LEVEL;
	s_filter_scr.next = NULL;

	s_filter_file.minlevel = loggingMinLevel;
	s_filter_file.maxlevel = LOGGING_MAX_LEVEL;
	s_filter_file.next = NULL;

	s_formatter.datefmt = "%Y-%m-%dT%H:%M:%SZ";
	s_formatter.fmt =
		"%(asctime)s	"
		"%(levelname)s	"
		"%(message)s";

#if defined(_WIN32) || defined (_WIN64)
	sprintf(logpath, "%s\\%s", cwd, PATH_METER_LOG);
#else
	sprintf(logpath, "%s/%s", cwd, PATH_METER_LOG);
#endif
	logfile = fopen(logpath, "a");

	s_handler_scr.name = "bgdm-scr";
	s_handler_scr.file = stdout;
	s_handler_scr.filter = &s_filter_scr;
	s_handler_scr.formatter = &s_formatter;
	s_handler_scr.next = logfile ? &s_handler_file : NULL;

	s_handler_file.name = "bgdm-file";
	s_handler_file.file = logfile;
	s_handler_file.filter = &s_filter_file;
	s_handler_file.formatter = &s_formatter;
	s_handler_file.next = NULL;

	s_logger.filter = &s_filter_scr;
	s_logger.handler = &s_handler_scr;

	logging_setlogger(&s_logger);

	if (!logfile)
		LOG_WARN("[server] Unable to open %s for writing.", logpath);

	return true;
}

// Runs the server.
int main(int argc, char** argv)
{
	static i8 cwd[MAX_PATH];

	// Get current working directory
#if defined(_WIN32) || defined(_WIN64)
	GetModuleFileNameA(NULL, cwd, MAX_PATH);
	PathRemoveFileSpecA(cwd);
	PathCombineA(g_dll_path, cwd, PATH_METER_DLL);
#else
	getcwd(cwd, MAX_PATH);
	sprintf(g_dll_path, "%s/%s", cwd, PATH_METER_DLL);
#endif

	i32 loggingLevel = -1;
	if (argc > 1 && atoi(argv[1])>=0)
		loggingLevel = (u32)abs(atoi(argv[1]));

	// Create the logger
	if (logger_create(cwd, loggingLevel))
		LOG_INFO("[server] logger started successfully, loglevel=%d", loggingLevel);

	// Open the localDB
	if (db_create(cwd) == SQLITE_OK)
		LOG_INFO("[server] Database engine started successfully");

	// Create the timer
	time_create();

	if (crypto_create() == false)
	{
#if defined(_WIN32) || defined (_WIN64)
		LOG_ERR("[server] error 0x%X: could not initialize crypto library", GetLastError());
#else
		LOG_ERR("[server] could not read private/public key pair");
#endif
		goto exit;
	}

	if (!meter_load()) {
		LOG_ERR("[server] error: unable to load %s", PATH_METER_DLL);
		goto exit;
	}

	Context ctx;
	if (context_create(&ctx) == false)
	{
		goto exit;
	}

	u16 srv_port = MSG_NETWORK_PORT;
	if (argc > 2 && atoi(argv[2]) && abs(atoi(argv[2]))<65536)
		srv_port = (u16)abs(atoi(argv[2]));
	if (network_create(srv_port) == false)
	{
		LOG_ERR("[server] error: could not initialize the network interface.");
		goto exit;
	}

	LOG_INFO("[server] waiting for incoming connections [UDP:%d]", srv_port);
	for (;;)
	{
		static i8 buf[UDP_MAX_PKT_SIZE];
		u32 addr, port;
		i32 len = network_recv(buf, sizeof(buf), &addr, &port);
		if (len < 0)
		{
#if defined(_WIN32) || defined (_WIN64)
			break;
#else
			LOG_CRIT("recvfrom() failed, error 0x%x", errno);
			continue;
#endif
		}

		if (len < 4)
		{
			continue;
		}

		// Update the timer and handle the message.
		i64 now = time_get();
		u32 type = *(u32*)buf;

		switch (type)
		{

		case MSG_TYPE_CLIENT_DAMAGE_UTF8:
		{
			if (len != sizeof(MsgClientDamageUTF8))
			{
				break;
			}

			MsgClientDamageUTF8* msg = (MsgClientDamageUTF8*)buf;
			msg_client_damage_utf8(&ctx, msg, addr, port, now);
		} break;

		case MSG_TYPE_CLIENT_UPDATE_BEGIN_UTF8:
		{
			if (len != sizeof(MsgClientUpdateBeginUTF8))
			{
				break;
			}

			MsgClientUpdateBeginUTF8* msg = (MsgClientUpdateBeginUTF8*)buf;
			msg_client_update_begin_utf8(&ctx, msg, addr, port, now);
		} break;

		case MSG_TYPE_CLIENT_UPDATE_PIECE:
		{
			if (len != sizeof(MsgClientUpdatePiece))
			{
				break;
			}

			MsgClientUpdatePiece* msg = (MsgClientUpdatePiece*)buf;
			msg_client_update_piece(&ctx, msg, addr, port, now);
		} break;

		}
	}

	LOG_CRIT("error: network interface failure");

exit:
	if (s_db)
		sqlite3_close(s_db);
	network_destroy();
	crypto_destroy();

	return 1;
}
