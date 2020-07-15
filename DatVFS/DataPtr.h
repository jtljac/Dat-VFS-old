#pragma once

class Counter {
	int count = 0;

	// Remove copy constructors, prevent reassign
	Counter(const Counter&) = delete;
	Counter& operator=(const Counter&) = delete;
public:
	Counter() {}

	/**
	 * Gets the count
	 * @return The Count
	 */
	int get() {
		return count;
	}

	void operator++()
	{
		count++;
	}

	void operator++(int)
	{
		count++;
	}

	void operator--()
	{
		count--;
	}
	void operator--(int)
	{
		count--;
	}

};

class DataPtr {
	// Pointer to the pointer that represents the data (needed so we can assign the data elsewhere)
	char** pointer = nullptr;
	Counter* counter = nullptr;

	// Pointer to the bool that represents whether the data is loaded (Needed so we can assign elsewhere)
	bool* loaded = nullptr;

	// The minimum number of owners before the data is cleaned up
	int minOwners = 1;

public:
	explicit DataPtr(char* Pointer = nullptr, int MinOwners = 0) {
		// Create the pointer to the data and assign it to the pointer to the pointer to the data (this might get confusing)
		pointer = new char*;
		*pointer = Pointer;

		counter = new Counter();
		(*counter)++;
		loaded = new bool(false);
		minOwners = MinOwners;
	}

	// Copy Constructor
	DataPtr(DataPtr& data) {
		pointer = data.pointer;
		counter = data.counter;
		loaded = data.loaded;
		(*counter)++;
	}

	~DataPtr() {
		// Remove this as an owner
		(*counter)--;

		// Check if we've hit the minimum amount of owners
		if (counter->get() == 0)
		{
			// Delete everything
			if (dataLoaded())
			{
				delete[] *pointer;
			}


			delete pointer;

			delete counter;

			delete loaded;

		}
		else if (counter->get() < minOwners)
		{
			if (dataLoaded()) {

				// Delete the data
				delete[] * pointer;
				*pointer = nullptr;
				*loaded = false;
			}
		}
	}

	char* operator->() {
		if (dataLoaded()) return *pointer;
		else return nullptr;
	}

	/**
	 * Gets the raw pointer
	 * @return A pointer to the data
	 */
	char* get() {
		return *pointer;
	}

	/**
	 * Sets the data pointer to point at the new data
	 * @param Data The data to
	 */
	void setData(char* Data) {
		*pointer = Data;
	}

	/**
	 * Sets the Loaded value for all shared pointers
	 * @param Load Whether the data is loaded
	 */
	void setLoaded(bool Load) {
		*loaded = Load;
	}

	/**
	 * Cleans up the data, deleting it and setting it to unloaded, without breaking the conditions for other DataPtr Instances
	 */
	void cleanup() {
		delete[] *pointer;
		*pointer = nullptr;
		setLoaded(false);
	}

	/**
	 * Gets whether the data is loaded
	 * @return whether the data is currently loaded
	 */
	bool dataLoaded() {
		return *loaded;
	}
};