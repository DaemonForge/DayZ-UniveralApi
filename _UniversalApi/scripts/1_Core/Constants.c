static const string UAPI_VERSION = "0.10.1";

static const int UAPI_SUCCESS = 200;
static const int UAPI_EMPTY = 204;
static const int UAPI_NOTSETUP = 424;//Used for discord requests only right now.
static const int UAPI_TIMEOUT = 408;
static const int UAPI_CLIENTERROR = 400;
static const int UAPI_SERVERERROR = 500;
static const int UAPI_ERROR = 418;
static const int UAPI_NOTFOUND = 404;
static const int UAPI_TOOEARLY = 425;
static const int UAPI_UNAUTHORIZED = 401;


static const int UAPI_DBSUCCESS = 200;
static const int UAPI_DBEMPTY = 204;
static const int UAPI_DBTIMEOUT = 408;
static const int UAPI_DBSERVERERROR = 500;


static const int UAPI_DBUNAUTHORIZED = 401;
static const int UAPI_DBERROR = 418;
static const int UAPI_DBTOOEARLY = 425;


static const int PLAYER_DB = 100;
static const int OBJECT_DB = 101;


class UpdateOpts {
	static string SET = "set";
	static string PULL = "pull";
	static string PUSH = "push";
	static string UNSET = "unset";
	static string MUL = "mul";
	static string RENAME = "rename";
	static string PULLALL = "pullAll";
}