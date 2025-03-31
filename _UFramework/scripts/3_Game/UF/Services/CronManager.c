/**
 * UCronManager Class
 *
 * This class manages a collection of scheduled functions (cron functions) by utilizing Unix time 
 * as a reference for execution timing. It supports registering functions that execute endlessly, 
 * until a specified Unix end time, until a maximum execution count is reached, or just once.
 *
 * Methods:
 *   - Init():
 *       Initializes the UCronManager by setting the starting Unix time reference, instantiating
 *       the cron functions array, and scheduling a periodic clean-up call to RemoveNull.
 *
 *   - onUpdate():
 *       Checks if the current Unix time has advanced and, if so, calls checkAndRun() to evaluate 
 *       and execute due cron functions.
 *
 *   - checkAndRun(int curTime):
 *       Iterates through the registered cron functions and for each function whose scheduled 
 *       time has come (or passed):
 *         - Executes the function using the system call queue.
 *         - Increments its execution count.
 *         - Schedules its removal if applicable based on its internal conditions.
 *
 *   - runEndless(int freqSeconds, Class obj, string fnName, Param params = NULL):
 *       Registers a cron function to be executed repeatedly every freqSeconds indefinitely.
 *
 *   - runEndTime(int freqSeconds, int endCallUnix, Class obj, string fnName, Param params = NULL):
 *       Registers a cron function to be executed every freqSeconds until a certain Unix time is reached.
 *
 *   - runEndCount(int freqSeconds, int maxCount, Class obj, string fnName, Param params = NULL):
 *       Registers a cron function to be executed every freqSeconds until it has been called maxCount times.
 *
 *   - runOnce(int nextRunUnix, Class obj, string fnName, Param params = NULL):
 *       Registers a cron function to be executed once at the specified Unix time.
 *
 *   - RemoveByFunc(UCronFunction cronFunc):
 *       Removes the specified cron function from the list of registered functions.
 *
 *   - Remove(Class obj, string fnName):
 *       Removes cron functions matching the provided object and function name from the registration.
 *
 *   - RemoveNull():
 *       Periodically checks and removes cron functions that are no longer valid.
 */

//This allows for me to better ensure timing of calls by using the unix time as a refrence

class UCronManager extends Managed {
	
	// The last recorded Unix time when a cron function was executed.
	protected int m_LastRunTime = 0;
	protected bool m_isInit = false;
	
	// Array storing all scheduled cron functions.
	protected autoptr array<autoptr UCronFunction> m_CronFunctions;
	
	/**
	 * Init
	 *
	 * Initializes the Cron Manager by:
	 * - Setting the starting Unix time reference.
	 * - Instantiating the array to hold cron functions.
	 * - Registering the RemoveNull method to be called endlessly every 15 minutes.
	 */
	void Init(){
		if (m_isInit) return;
		m_isInit = true;
		m_LastRunTime = UUtil.GetUnixInt();
		m_CronFunctions = new array<autoptr UCronFunction>;
		// Schedule RemoveNull to be called every (15 * 60) seconds.
		this.runEndless((15 * 60), this, "RemoveNull");
	}
	
	/**
	 * onUpdate
	 *
	 * Called periodically to update the manager.
	 * Checks if the system time has advanced, and if so, processes any due cron function by calling checkAndRun.
	 */
	void onUpdate(){
		int currentRunTime = UUtil.GetUnixInt();
		// Only update if Unix time has advanced.
		if (currentRunTime > m_LastRunTime) {
			checkAndRun(currentRunTime);
			m_LastRunTime = UUtil.GetUnixInt();
		}		
	}
	
	/**
	 * checkAndRun
	 *
	 * Iterates over the registered cron functions, and for each function:
	 * - Checks if its scheduled execution time is reached.
	 * - Executes it if due.
	 * - Determines if the function should be removed afterwards.
	 *
	 * @param curTime  The current Unix time.
	 */
	void checkAndRun(int curTime){
		// Exit early if there are no cron functions.
		if (!m_CronFunctions) return;
		if (m_CronFunctions.Count() < 1) return;
		
		// Iterate through all registered cron functions.
		foreach(UCronFunction cronFunc : m_CronFunctions){
			Class obj;           // Object on which the function will be executed.
			string funcName;     // Name of the function to execute.
			Param params;        // Parameters for the function call.
			bool shouldDelete = false; // Flag indicating if the function should be removed after execution.
			
			// Check if the scheduled time for the function is due.
			if (cronFunc.shouldAttemptCall(curTime, obj, funcName, params, shouldDelete)){
				Print("[UF] [Cron] Running Function " + funcName + " @ " + curTime);
				// Enqueue the function call via the system call queue.
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallByName(obj, funcName, params);
			}
			// If flagged for removal, schedule the removal call.
			if (shouldDelete){
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.RemoveByFunc, cronFunc);
			}
		}
	}
	
	/**
	 * runEndless
	 *
	 * Registers a cron function which executes repeatedly at a fixed frequency.
	 *
	 * @param freqSeconds  Frequency in seconds between executions.
	 * @param obj          The target object that owns the function.
	 * @param fnName       The name of the function to execute.
	 * @param params       Optional parameters for the function.
	 */
	void runEndless(int freqSeconds, Class obj, string fnName, Param params = NULL) {
		Print("[UF] [Cron] Registering Endless Function " + fnName + " every " + freqSeconds);
		int idx = m_CronFunctions.Insert(new UCronFunction(freqSeconds, obj, fnName, params));
	}
	
	/**
	 * runEndTime
	 *
	 * Registers a cron function for periodic execution until a specified Unix end time.
	 *
	 * @param freqSeconds  Frequency in seconds between executions.
	 * @param endCallUnix  Unix time after which the function will no longer be scheduled.
	 * @param obj          The target object that owns the function.
	 * @param fnName       The name of the function to execute.
	 * @param params       Optional parameters for the function.
	 */
	void runEndTime(int freqSeconds, int endCallUnix, Class obj, string fnName, Param params = NULL) {
		Print("[UF] [Cron] Registering Function w/ Endtime " + fnName + " every " + freqSeconds + " end at " + endCallUnix);
		int idx = m_CronFunctions.Insert(new UCronFunction(freqSeconds, obj, fnName, params));
		// Set the end time for scheduled executions.
		m_CronFunctions.Get(idx).setEndTime(endCallUnix);
	}
	
	/**
	 * runEndCount
	 *
	 * Registers a cron function for periodic execution until it reaches a maximum count of executions.
	 *
	 * @param freqSeconds  Frequency in seconds between executions.
	 * @param maxCount     Maximum allowed executions before removal.
	 * @param obj          The target object that owns the function.
	 * @param fnName       The name of the function to execute.
	 * @param params       Optional parameters for the function.
	 */
	void runEndCount(int freqSeconds, int maxCount, Class obj, string fnName, Param params = NULL) {
		Print("[UF] [Cron] Registering Function w/ maxCount " + fnName + " every " + freqSeconds + " end after " + maxCount);
		int idx = m_CronFunctions.Insert(new UCronFunction(freqSeconds, obj, fnName, params));
		// Set the maximum execution count allowed.
		m_CronFunctions.Get(idx).setMaxCount(maxCount);
	}
	
	/**
	 * runOnce
	 *
	 * Registers a cron function to be executed only once at a specified Unix time.
	 *
	 * @param nextRunUnix  The Unix time at which to execute the function.
	 * @param obj          The target object that owns the function.
	 * @param fnName       The name of the function to execute.
	 * @param params       Optional parameters for the function.
	 */
	void runOnce(int nextRunUnix, Class obj, string fnName, Param params = NULL) {
		Print("[UF] [Cron] Registering run Once Function " + fnName + " run at " + nextRunUnix);
		// Use a negative frequency to indicate one-time execution.
		int idx = m_CronFunctions.Insert(new UCronFunction(-1, obj, fnName, params));
		// Manually set the scheduled time for execution.
		m_CronFunctions.Get(idx).setNextCall(nextRunUnix);
	}
	
	/**
	 * RemoveByFunc
	 *
	 * Removes a specific cron function from the registered list.
	 *
	 * @param cronFunc  The cron function instance to remove.
	 */
	void RemoveByFunc(UCronFunction cronFunc){
		Print("[UF] [Cron] Removing Function " + cronFunc.GetFuncName());
		m_CronFunctions.RemoveItem(cronFunc);
	}
	
	/**
	 * Remove
	 *
	 * Removes any cron functions matching a specific object and function name.
	 *
	 * @param obj      The target object.
	 * @param fnName   The name of the function to remove.
	 */
	void Remove(Class obj, string fnName){
		Print("[UF] [Cron] Removing Function " + fnName);
		if(!obj) return;
		// Iterate and remove each matching cron function.
		foreach(UCronFunction cronFunc : m_CronFunctions){
			if (cronFunc.is(obj, fnName)) RemoveByFunc(cronFunc);
		}
	}
	
	/**
	 * RemoveNull
	 *
	 * Iterates over the cron functions and removes any that no longer have a valid target object.
	 * This is typically used as a clean-up mechanism.
	 */
	void RemoveNull(){
		// Check if there are any functions to process.
		if (m_CronFunctions.Count() < 1) return;
		// Iterate through the cron functions and remove invalid ones.
		foreach(UCronFunction cronFunc : m_CronFunctions){
			if (!cronFunc.isValid()) {
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.RemoveByFunc, cronFunc);
			}
		}
	}
}


/**
 * UCronFunction Class
 *
 * This class represents an individual scheduled function, encapsulating the necessary data to 
 * control its execution timing, including the target object, function name, parameters, and various
 * criteria for when the function should be scheduled or terminated.
 *
 * Fields:
 *   - m_obj:           The target object on which the function will be called.
 *   - m_funcName:      The name of the function to be executed.
 *   - m_params:        Parameters to be passed to the function upon execution.
 *   - m_freq:          The frequency (in seconds) at which the function should be executed. 
 *                      A negative value indicates a one-time call.
 *   - m_nextCall:      The Unix time when the function is next scheduled to run.
 *   - m_endCall:       The Unix time after which the function should cease execution.
 *   - m_maxCount:      Maximum allowed executions; when reached, the function is marked for deletion.
 *   - m_curCount:      Current number of times the function has been executed.
 *
 * Methods:
 *   - UCronFunction(int freq, Class obj, string funcName, Param params):
 *       Constructor that initializes a new cron function with the given execution frequency, 
 *       target object, function name, and parameters.
 *
 *   - GetFuncName():
 *       Returns the function name associated with this cron function.
 *
 *   - shouldAttemptCall(int curTime, out Class obj, out string funcName, out Param params, out bool shouldDelete):
 *       Checks if the cron function should be executed based on the current Unix time.
 *       If the scheduled time has been reached:
 *         - It prepares the parameters and output values required for execution.
 *         - Increments the call counter.
 *         - Calls setNextbyCurent() to update the scheduling.
 *         - Returns true indicating that the function should be executed.
 *
 *   - setNextbyCurent(int curTime):
 *       Determines and sets the next scheduled call time based on the current time and frequency.
 *       Evaluates conditions such as frequency value, maximum execution count, and end time.
 *       Returns true if the function should be deleted (e.g., one-time execution, count exceeded, or past end time).
 *
 *   - is(Class obj, string funcName):
 *       Compares the current function's target object and function name with the provided values.
 *       Returns true if both match; false otherwise.
 *
 *   - isValid():
 *       Checks the validity of the target object (m_obj); returns false if m_obj is null.
 *
 *   - setNextCall(int nextCall):
 *       Manually sets the Unix time when the function is scheduled to be invoked next.
 *
 *   - setMaxCount(int maxCount):
 *       Updates the maximum number of times this function is allowed to execute.
 *
 *   - setEndTime(int endTime):
 *       Sets the Unix end time after which the function should no longer be scheduled for execution.
 */
class UCronFunction extends Managed {

	// Protected member variables storing the target object, function name, parameters,
	// frequency of calls, next scheduled call time, end time, maximum allowed call count,
	// and the current call count.
	protected Class m_obj;
	protected string m_funcName;
	protected Param m_params;
	protected int m_freq;
	protected int m_nextCall;
	protected int m_endCall;
	protected int m_maxCount = -1;
	protected int m_curCount = 0;
	
	/**
	 * Constructor UCronFunction
	 *
	 * Initializes a new instance of the UCronFunction class with the specified frequency,
	 * target object, function name, and parameters.
	 *
	 * @param freq       The frequency (in seconds) at which the function should be executed.
	 *                   A negative value implies a one-time execution.
	 * @param obj        The target object on which the function will be called.
	 * @param funcName   The name of the function to invoke.
	 * @param params     The parameters to pass to the function (optional).
	 */
	void UCronFunction(int freq, Class obj, string funcName, Param params){
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
		m_params = params;
		m_freq = freq;
		m_nextCall = UUtil.GetUnixInt() + freq;
	}
	
	/**
	 * GetFuncName
	 *
	 * Retrieves the function name associated with this cron function.
	 *
	 * @return The function name as a string.
	 */
	string GetFuncName(){
		return m_funcName;
	}
	
	/**
	 * shouldAttemptCall
	 *
	 * Evaluates whether the cron function should be executed based on the current Unix time.
	 * If the scheduled time (m_nextCall) is reached or passed, the function is prepared for execution.
	 *
	 * @param curTime     The current Unix time.
	 * @param obj         (Output) The target object for function invocation.
	 * @param funcName    (Output) The name of the function to be called.
	 * @param params      (Output) The parameters to pass to the function.
	 * @param shouldDelete (Output) Indicates whether the function should be removed after execution.
	 *
	 * @return True if the function should be called; false otherwise.
	 */
	bool shouldAttemptCall(int curTime, out Class obj, out string funcName, out Param params, out bool shouldDelete){
		shouldDelete = false;
		// If the next scheduled call time is reached
		if (m_nextCall <= curTime){
			// Update the scheduling information and determine if the function should be deleted
			shouldDelete = setNextbyCurent(curTime);
			funcName = m_funcName;
			params = m_params;
			m_curCount++;
			// Attempt to cast and return the target object
			return Class.CastTo(obj, m_obj);
		}
		return false;
	}
	
	/**
	 * setNextbyCurent
	 *
	 * Determines and sets the next scheduled call time based on the current time and defined frequency.
	 * Evaluates conditions such as one-time execution, maximum call count reached, or exceeding the end time.
	 *
	 * @param curTime     The current Unix time.
	 *
	 * @return True if the function should be deleted (i.e., no further calls are scheduled); false otherwise.
	 */
	protected bool setNextbyCurent(int curTime){
		// For one-time execution (or negative frequency), mark for deletion.
		if (m_freq <= 0) return true;
		// If a maximum count is defined and reached, mark for deletion.
		if (m_maxCount > 0 && m_curCount >= m_maxCount) return true;
		// If the current time exceeds the specified end time, mark for deletion.
		if (curTime >= m_endCall) return true;
		// Schedule the next call based on the frequency.
		m_nextCall = curTime + m_freq;
		return false;
	}
	
	/**
	 * is
	 *
	 * Checks if this cron function instance matches the provided target object and function name.
	 *
	 * @param obj       The target object to compare.
	 * @param funcName  The name of the function to compare.
	 *
	 * @return True if both the target object and function name match; false otherwise.
	 */
	bool is(Class obj, string funcName){
		return (obj == m_obj && funcName == m_funcName);
	}
	
	/**
	 * isValid
	 *
	 * Validates the cron function by ensuring that the target object is not null.
	 *
	 * @return True if the target object is valid; false otherwise.
	 */
	bool isValid(){
		if (!m_obj) return false;
		return true;
	}
	
	/**
	 * setNextCall
	 *
	 * Manually sets the next scheduled call time for the function.
	 *
	 * @param nextCall  The Unix time when the function should next be executed.
	 */
	void setNextCall(int nextCall){
		m_nextCall = nextCall;
	}
	
	/**
	 * setMaxCount
	 *
	 * Defines the maximum number of executions allowed for this cron function.
	 *
	 * @param maxCount  The maximum call count before the function is marked for deletion.
	 */
	void setMaxCount(int maxCount){
		m_maxCount = maxCount;
	}	
	
	/**
	 * setEndTime
	 *
	 * Specifies the Unix time after which the function should no longer be executed.
	 *
	 * @param endTime  The Unix time representing the end threshold.
	 */
	void setEndTime(int endTime){
		m_endCall = endTime;
	}
}