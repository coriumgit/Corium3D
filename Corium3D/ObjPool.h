#pragma once

#include "IdxPool.h"

// TODO: fix the code duplication on the acquire methods
namespace Corium3DUtils {

	template <class T>
	class ObjPool {
	public:
		ObjPool(unsigned int maxSz);		
		ObjPool(ObjPool const& objPool) = delete;
		~ObjPool();
		T* acquire();
		template <class ...Args>
		T* acquire(Args&&... args);
		template <class S>
		T* acquire(S const& initObj);
		template <class S>
		T* acquire(S& initObj);
		T* acquire(T const& initObj);
		void release(T* obj);
		unsigned int getMaxSz() const { return maxSz; }
		unsigned int getAcquiredObjsNr() const { return idxPool->getAcquiredIdxsNr(); }
		bool isFull() const { return maxSz == idxPool->getAcquiredIdxsNr(); }

	private:
		const unsigned int maxSz;
		char* memPtr;		
		IdxPool* idxPool;
	};

	template <class T>
	ObjPool<T>::ObjPool(unsigned int _maxSz) : maxSz(_maxSz), memPtr(new char[sizeof(T) * maxSz]), idxPool(new IdxPool(maxSz)) {}

	// TODO: call destructors of all unreleased objects (?)
	template <class T>
	ObjPool<T>::~ObjPool() {
		delete idxPool;
		delete[] memPtr;		
	}

	template <class T>
	T* ObjPool<T>::acquire() { 
		try {
			return new((T*)memPtr + idxPool->acquire()) T();
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}	

	template <class T>
	template <class ...Args>
	T* ObjPool<T>::acquire(Args&&... args) {
		try {
			return new((T*)memPtr + idxPool->acquire()) T(std::forward<Args>(args)...);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	template <class S>
	T* ObjPool<T>::acquire(S const& initObj) {
		try {
			return new((T*)memPtr + idxPool->acquire()) T(initObj);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	template <class S>
	T* ObjPool<T>::acquire(S& initObj) {
		try {
			return new((T*)memPtr + idxPool->acquire()) T(initObj);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	T* ObjPool<T>::acquire(T const& initObj) { 
		try {
			return new((T*)memPtr + idxPool->acquire()) T(initObj);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}
	
	template <class T>
	void ObjPool<T>::release(T* obj) {
#if DEBUG
		if (obj < (T*)memPtr)
			throw std::invalid_argument("Object's address is lower than the pool's addresses space !");
#endif
		unsigned int objPoolSlotIdx = (unsigned int)(obj - (T*)memPtr);
		try {
			idxPool->release(objPoolSlotIdx);
		}
		catch (std::invalid_argument) {
			throw std::invalid_argument("Object was not previously acquired.");
		}
		((T*)memPtr)[objPoolSlotIdx].~T();
	}

	template <class T>
	class ObjPoolIteratable {
	public:
		// REMINDER: Iteration order is unspecified
		class ObjPoolIt {
		public:
			ObjPoolIt(ObjPoolIteratable& _objPool);
			void reset();
			T& next();
			void removeCurr();
			bool hasNext() const;

		private:
			ObjPoolIteratable<T>& objPool;
			typename ObjPoolIteratable::DataRec* curr;
			typename ObjPoolIteratable::DataRec* prev;
		};

		ObjPoolIteratable(unsigned int maxSz);
		ObjPoolIteratable(ObjPoolIteratable const& objPool) = delete;
		~ObjPoolIteratable();
		T* acquire();		
		T* acquire(T const& initObj);
		template <class ...Args>
		T* acquire(Args... args);
		template <class S>
		T* acquire(S const& initObj);
		template <class S>
		T* acquire(S& initObj);		
		void release(T* obj);
		unsigned int getObjIdxInPool(T const* obj);
		unsigned int getMaxSz() const { return maxSz; }
		unsigned int getAcquiredObjsNr() const { return idxPool->getAcquiredIdxsNr(); }
		bool isFull() const { return maxSz == idxPool->getAcquiredIdxsNr(); }
		//T* operator[](unsigned int idx);

	private:
		struct DataRec {
			DataRec() : data() {}
			DataRec(T const& initObj) : data(initObj) {}

			template <class ...Args>
			DataRec(Args... args) : data(args...) {}

			template <class S>
			DataRec(S const& initObj) : data(initObj) {}			

			T data;
			DataRec* prev = NULL;
			DataRec* next = NULL;
		};

		const unsigned int maxSz;
		char* memPtr;
		IdxPool* idxPool;
		DataRec* listHead = NULL;
		DataRec* listTail = NULL;
	};

	template <class T>
	ObjPoolIteratable<T>::ObjPoolIteratable(unsigned int _maxSz) : maxSz(_maxSz), memPtr(new char[(sizeof(DataRec) + 2 * sizeof(DataRec*))*maxSz]), idxPool(new IdxPool(maxSz)) {}

	template <class T>
	ObjPoolIteratable<T>::~ObjPoolIteratable() {
		delete idxPool;
		delete[] memPtr;
	}

	template <class T>
	T* ObjPoolIteratable<T>::acquire() {
		try {
			DataRec* dataPtr = new((DataRec*)memPtr + idxPool->acquire()) DataRec();
			if (listTail != NULL) {
				dataPtr->prev = listTail;
				listTail = dataPtr;
			}
			else
				listHead = listTail = dataPtr;

			return &(dataPtr->data);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	template <class ...Args>
	T* ObjPoolIteratable<T>::acquire(Args... args) {
		try {			
			DataRec* dataPtr = new((DataRec*)memPtr + idxPool->acquire()) DataRec(args...);
			if (listTail != NULL) {
				dataPtr->prev = listTail;
				listTail->next = dataPtr;
				listTail = dataPtr;
			}
			else
				listHead = listTail = dataPtr;

			return &(dataPtr->data);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	template <class S>
	T* ObjPoolIteratable<T>::acquire(S const& initObj) {
		try {
			DataRec* dataPtr = new((DataRec*)memPtr + idxPool->acquire()) DataRec(initObj);
			if (listTail != NULL) {
				dataPtr->prev = listTail;
				listTail->next = dataPtr;
				listTail = dataPtr;
			}
			else
				listHead = listTail = dataPtr;

			return &(dataPtr->data);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	template <class S>
	T* ObjPoolIteratable<T>::acquire(S& initObj) {
		try {
			DataRec* dataPtr = new((DataRec*)memPtr + idxPool->acquire()) DataRec(initObj);
			if (listTail != NULL) {
				dataPtr->prev = listTail;
				listTail->next = dataPtr;
				listTail = dataPtr;
			}
			else
				listHead = listTail = dataPtr;

			return &(dataPtr->data);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	T* ObjPoolIteratable<T>::acquire(T const& initObj) {
		try {
			DataRec* dataPtr = new((DataRec*)memPtr + idxPool->acquire()) DataRec(initObj);
			if (listTail != NULL) {
				dataPtr->prev = listTail;
				listTail->next = dataPtr;
				listTail = dataPtr;
			}
			else
				listHead = listTail = dataPtr;

			return &(dataPtr->data);
		}
		catch (std::underflow_error) {
			throw std::underflow_error("No more objects.");
		}
	}

	template <class T>
	void ObjPoolIteratable<T>::release(T* obj) {
		DataRec* releasedDataRec = (DataRec*)obj;
#if DEBUG
		if (releasedDataRec < (DataRec*)memPtr)
			throw std::invalid_argument("Object's address is lower than the pool's addresses space !");
#endif
		unsigned int objPoolSlotIdx = (unsigned int)(releasedDataRec - (DataRec*)memPtr);
		
		try {
			idxPool->release(objPoolSlotIdx);
		}
		catch (std::invalid_argument) {
			throw std::invalid_argument("Object was not previously acquired.");
		}

		if (releasedDataRec->prev)
			releasedDataRec->prev->next = releasedDataRec->next;
		else
			listHead = releasedDataRec->next;

		if (releasedDataRec->next)
			releasedDataRec->next->prev = releasedDataRec->prev;
		else
			listTail = releasedDataRec->prev;

		releasedDataRec->prev = NULL;
		releasedDataRec->next = NULL;

		releasedDataRec->data.~T();
	}

	template <class T>
	unsigned int ObjPoolIteratable<T>::getObjIdxInPool(T const* obj) {		
#if DEBUG
		unsigned int addressIdxInPool = (unsigned int)((DataRec*)obj - (DataRec*)memPtr);
		if (addressIdxInPool < 0)
			throw std::invalid_argument("Object's address is lower than the pool's addresses space !");
		if (!idxPool->isAcquired(addressIdxInPool))
			throw std::invalid_argument("Object was not previously acquired.");
#endif

		return (unsigned int)((DataRec*)obj - (DataRec*)memPtr);
	}

	template <class T>
	ObjPoolIteratable<T>::ObjPoolIt::ObjPoolIt(ObjPoolIteratable<T>& _objPool) : objPool(_objPool), curr(objPool.listHead), prev(NULL) {}
	
	template <class T>
	void ObjPoolIteratable<T>::ObjPoolIt::reset() {
		curr = objPool.listHead;
		prev = NULL;
	}

	template <class T>
	T& ObjPoolIteratable<T>::ObjPoolIt::next() {
#if DEBUG
		if (!hasNext())
			throw std::out_of_range("Iterator was nexted while done.");
#endif

		T& currData = curr->data;
		prev = curr;
		curr = curr->next;

		return currData;
	}

	template <class T>
	void ObjPoolIteratable<T>::ObjPoolIt::removeCurr() {
#if DEBUG
		if (prev == NULL)
			throw std::out_of_range("Iterator was asked to remove current elemnt before beginning.");
#endif
		objPool.release(&prev->data);
		if (curr)
			prev = curr->prev;
	}

	template <class T>
	bool ObjPoolIteratable<T>::ObjPoolIt::hasNext() const {
		return curr != NULL;
	}

/*
	template <class T>
	T* ObjPoolIteratable<T>::operator[](unsigned int idx) {
#if DEBUG
		if (!idxPool->isAcquired(idx))
			throw std::invalid_argument("Element at given idx was not previously acquired.");
#endif
		return (T*)((DataRec*)memPtr + idx);
	}
*/

}
