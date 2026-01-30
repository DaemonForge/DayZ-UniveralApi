/**
 * Modded Math class with quantum random number pool.
 * 
 * Provides higher-quality random numbers than vanilla using a server-provided
 * quantum random pool. Falls back to vanilla when pool is empty.
 * 
 * @usage int random = Math.QRandomInt(1, 100);
 * @usage float chance = Math.QRandomFloat(0.0, 1.0);
 * @usage bool flip = Math.QRandomFlip();
 * 
 * @note Pool auto-refills via U().CheckAndRenewQRandom()
 * @note Use QRandom methods instead of vanilla Random for better randomness
 */
modded class Math
{
	
	protected static autoptr TIntArray m_QRandomNumbers = new TIntArray;
	
	/**
	 * Adds quantum random numbers to the pool.
	 * 
	 * @param numbers Array of random integers from API
	 * 
	 * @note Internal use - called by UFramework automatically
	 * @note Do not call manually - use U().CheckAndRenewQRandom() instead
	 */
	static void AddQRandomNumber(TIntArray numbers){
		if (!m_QRandomNumbers){
			m_QRandomNumbers = new TIntArray;
		}
		m_QRandomNumbers.InsertAll(numbers);
	}
	
	/**
	 * Gets count of remaining random numbers in pool.
	 * 
	 * @return Number of random integers available
	 * 
	 * @usage if (Math.QRandomRemaining() < 1000) { U().CheckAndRenewQRandom(); }
	 */
	static int QRandomRemaining(){
		if (!m_QRandomNumbers){
			return 0;
		}
		return m_QRandomNumbers.Count();
	}
	
	//Gets the number and removes it from the array
	protected static int GetAndRemoveNumber(){
		int idx = m_QRandomNumbers.GetRandomIndex();
		int number = m_QRandomNumbers.Get(idx);
		m_QRandomNumbers.Remove(idx);
		return number;
	}
	
	/**
	 * Gets random integer from full int range.
	 * 
	 * @return Random int between int.MIN and int.MAX
	 * 
	 * @note Falls back to vanilla RandomInt() if pool empty
	 * @note Prefer QRandomInt() with explicit range for most uses
	 */
	static int QRandom(){
		if (QRandomRemaining() <= 0){
			//Error2("[UF] QRandom", "Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return RandomInt(int.MIN, int.MAX);
		}
		int number = GetAndRemoveNumber();
		return number;
	}
	
	/**
	 * Gets random integer within specified range.
	 * 
	 * @param min Lower bound (inclusive, default: 0)
	 * @param max Upper bound (inclusive, default: int.MAX)
	 * @return Random integer between min and max
	 * 
	 * @usage int dice = Math.QRandomInt(1, 6);
	 * @note Falls back to vanilla RandomInt() if pool empty
	 * @note For ranges > 10000, consider QRandomFloat() for better distribution
	 */
	static int QRandomInt(int min = 0, int max = int.MAX){
		if (QRandomRemaining() <= 0){
			//Error2("[UF] QRandomInt", "Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return RandomInt(min, max);
		}
		if (min == max){
			return min;
		}
		if (max < min){
			int tmp = max;
			max = min;
			min = tmp;
		}
		int number = Math.AbsInt(GetAndRemoveNumber());
		if (min == 0 && max == int.MAX){
			return number;
		}
		
		int diff = max - min;
		int randomNum = number % (diff + 1);
		
		return min + randomNum;
	}
	
	/**
	 * Gets random float within specified range.
	 * 
	 * @param min Lower bound (default: 0.0)
	 * @param max Upper bound (default: 1.0)
	 * @return Random float between min and max
	 * 
	 * @usage float chance = Math.QRandomFloat(0.0, 1.0);
	 * @usage float damage = Math.QRandomFloat(50.0, 100.0);
	 * @note Falls back to vanilla RandomFloat() if pool empty
	 */
	static float QRandomFloat(float min = 0, float max = 1){
		if (QRandomRemaining() <= 0){
			//Error2("[UF] QRandomFloat", "Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return RandomFloat(min, max);
		}
		if (min == max){
			return min;
		}
		if (max < min){
			float tmp = max;
			max = min;
			min = tmp;
		}
		int number = Math.AbsInt(GetAndRemoveNumber());
		float num = number / int.MAX;
		float diff = max - min;
		float dnum = diff * num;
		return  dnum + min;
	}
	
	/**
	 * Gets random boolean (coin flip).
	 * 
	 * @return Random true or false
	 * 
	 * @usage if (Math.QRandomFlip()) { /* 50% chance */ }
	 * @note Falls back to vanilla random if pool empty
	 */
	static bool QRandomFlip(){
		if (QRandomRemaining() <= 0){
			//Error2("[UF] QRandomFlip", "Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			int retval = RandomInt(1, int.MAX) % 2;
			return ( retval != 0);
		}
		int number = Math.AbsInt(GetAndRemoveNumber());
		int reval = number % 2;
		return (reval != 0);
		
	}
}