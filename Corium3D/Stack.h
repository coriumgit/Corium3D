#pragma once

namespace Corium3DUtils {

	template <class T>
	class Stack {
	public:
		Stack(unsigned int _maxSz) : maxSz(_maxSz) {
			lmnts = new T*[maxSz];
		}

		~Stack() { delete[] lmnts; }

		void push(T const* lmnt) {
#if DEBUG
			if (currLmnt == maxSz) throw std::overflow_error();
#endif
			lmnts[nextLmnt++] = lmnt;
		}

		T* pop() {
#if DEBUG
			if (isEmpty()) throw std::underflow_error();
#endif
			return lmnts[--nextLmnt];
		}

		T* top() {
#if DEBUG
			if (isEmpty()) throw std::underflow_error();
#endif
			return lmnts[nextLmnt - 1];
		}

		void clear() { nextLmnt = 0;}

		bool isEmpty() { return nextLmnt == 0;}
		
	private:
		T** const lmnts;
		unsigned int maxSz;
		unsigned int nextLmnt = 0;
	};

} // namespace Corium3DUtils