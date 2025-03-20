/**
 * Class: Math (modded)
 *
 * Description:
 *   This class extends the basic Math functionality by providing a queue-based
 *   quantum-random number generator. It uses an internal pool (m_QRandomNumbers)
 *   to produce random integer, float, and boolean values. When the pool becomes empty,
 *   the implementation falls back to the vanilla random methods, this is done since the
 *   vanilla random methods are not very well designed and arn't very random.
 *
 * Properties:
 *   - m_QRandomNumbers:
 *       A static, protected integer array that holds a pool of random numbers for use
 *       by the various random generation methods.
 *
 * Methods:
 *
 *   - AddQRandomNumber(TIntArray numbers):
 *       Description:
 *         Inserts a set of integers into the internal random number pool.
 *       Parameters:
 *         - numbers: An array of integers to be added to the pool.
 *       Remarks:
 *         Should not be called directly; use U().CheckAndRenewQRandom() for automatic management.
 *
 *   - QRandomRemaining():
 *       Description:
 *         Returns the number of remaining random numbers in the pool.
 *       Returns:
 *         - int: The count of available random numbers.
 *
 *   - GetAndRemoveNumber():
 *       Description:
 *         Retrieves a random number from the pool by selecting a random index,
 *         returns the number at that index, and removes it from the pool.
 *       Returns:
 *         - int: The retrieved random number.
 *       Access Level:
 *         Protected helper method.
 *
 *   - QRandom():
 *       Description:
 *         Returns a pseudo-random number. If the pool is not empty, a number is
 *         retrieved from it; otherwise, it uses the vanilla random method to generate a number.
 *       Returns:
 *         - int: A pseudo-random integer.
 *
 *   - QRandomInt(int min = 0, int max = int.MAX):
 *       Description:
 *         Returns a pseudo-random integer within the specified range. It uses a number
 *         from the pool to generate a value between min and max by applying the modulus operator.
 *       Parameters:
 *         - min: The lower bound of the return value range (default is 0).
 *         - max: The upper bound of the return value range (default is int.MAX).
 *       Returns:
 *         - int: The generated random integer within the specified range.
 *       Remarks:
 *         If the internal pool is empty or min equals max, the traditional random method is used.
 *
 *   - QRandomFloat(float min = 0, float max = 1):
 *       Description:
 *         Returns a pseudo-random floating-point number within the specified range.
 *         A number from the pool is used to calculate the float value based upon int.MAX normalization.
 *       Parameters:
 *         - min: The lower bound of the return value range (default is 0.0).
 *         - max: The upper bound of the return value range (default is 1.0).
 *       Returns:
 *         - float: The generated random float within the specified range.
 *       Remarks:
 *         Similar to QRandomInt, defaults to vanilla random when the pool is empty.
 *
 *   - QRandomFlip():
 *       Description:
 *         Returns a pseudo-random boolean value. It determines the boolean outcome by
 *         checking the parity of a number retrieved from the pool.
 *       Returns:
 *         - bool: True or false determined randomly.
 *       Remarks:
 *         Falls back to the vanilla random method if the pool is depleted.
 */
modded class Math
{
	
	protected static ref TIntArray m_QRandomNumbers = new TIntArray;
	
	//Adds a new array shouldn't be called manually Use U().CheckAndRenewQRandom();
	static void AddQRandomNumber(TIntArray numbers){
		if (!m_QRandomNumbers){
			m_QRandomNumbers = new TIntArray;
		}
		m_QRandomNumbers.InsertAll(numbers);
	}
	
	//returns the remaining Random numbers to choose from
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
	
	//returns a random number between int.MAX and int.MIN
	static int QRandom(){
		if (QRandomRemaining() <= 0){
			//Error2("[UF] QRandom", "Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return RandomInt(int.MIN, int.MAX);
		}
		int number = GetAndRemoveNumber();
		return number;
	}
	
	//returns a random integer max difference between numbers is int.MAX(2147483647)
	//Unless returning a number between 0 and int.MAX exactly I would recomend not doing more than a difference of 10,000(ish) use random float instead
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
	
	//returns a random float
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
	
	//returns a random true or false value
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