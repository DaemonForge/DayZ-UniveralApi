static const string UAPI_VERSION = "1.1.0";

static const int UAPI_SUCCESS = 200;
static const int UAPI_EMPTY = 204; //Means response was empty or query result returned no results
static const int UAPI_NOTSETUP = 424; //Used for discord requests only right now.
static const int UAPI_TIMEOUT = 408;
static const int UAPI_CLIENTERROR = 400;
static const int UAPI_SERVERERROR = 500;
static const int UAPI_ERROR = 418;
static const int UAPI_JSONERROR = 406;
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
	static string SET = "set"; // `set` to set the value of an element
	static string PULL = "pull"; // `pull` to pull a value out of an array
	static string PUSH = "push"; // `push` to push a value into an array
	static string UNSET = "unset";// `unset` to remove an element from the database
	static string MUL = "mul"; // `mul` to mulitply an element by the value in the database
	static string RENAME = "rename";// `rename` to rename an element in the database
	static string PULLALL = "pullAll";// `pullAll` to empty an array
}