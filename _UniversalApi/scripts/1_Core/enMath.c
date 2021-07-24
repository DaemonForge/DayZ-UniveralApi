modded class Math
{
	
	protected static ref TIntArray m_QRandomNumbers = new TIntArray;
	
	
	static void AddQRandomNumber(TIntArray numbers){
		if (!m_QRandomNumbers){
			m_QRandomNumbers = new TIntArray;
		}
		//Print("[UAPI] Adding " + numbers.Count() + " more numbers to the array");
		m_QRandomNumbers.InsertAll(numbers);
	}
	
	static int RemainingQRandom(){
		if (!m_QRandomNumbers){
			return 0;
		}
		return m_QRandomNumbers.Count();
	}
	
	protected static int GetAndRemoveNumber(){
		int idx = m_QRandomNumbers.GetRandomIndex();
		int number = m_QRandomNumbers.Get(idx);
		m_QRandomNumbers.Remove(idx);
		return number;
	}
	
	
	static int QRandomInt(int min = 0, int max = 65535){
		if (!m_QRandomNumbers || m_QRandomNumbers.Count() <= 0){
			Print("[UAPI] QRandomInt Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			//m_QRandomNumbers.Debug();
			return RandomInt(min, max);
		}
		int number = GetAndRemoveNumber();
		if (min == 0 && max == 65535){
			return number;
		}
		
		float num = number / 65535;
		int diff = max - min;
		int dnum = diff * num;
		return  Round( dnum + min);
	}
	
	static float QRandomFloat(float min = 0, float max = 1){
		if (!m_QRandomNumbers || m_QRandomNumbers.Count() <= 0){
			Print("[UAPI] QRandomFloat Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return RandomFloat(min, max);
		}
		int number = GetAndRemoveNumber();
		float num = number / 65535;
		float diff = max - min;
		float dnum = diff * num;
		return  dnum + min;
	}
	
	static bool QRandomFlip(){
		if (!m_QRandomNumbers || m_QRandomNumbers.Count() <= 0){
			Print("[UAPI] QRandomFlip Q RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			return (RandomInt(1, 8) >= 5);
		}
		int number = GetAndRemoveNumber();
		int reval = number % 2;
		return (reval != 0);
		
	}
}