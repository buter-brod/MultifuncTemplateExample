#include <iostream>
#include <string>

class WritableBase {
public:

	WritableBase(const void* ptr, const bool own) : _ptr(ptr), _own(own) {}
	virtual ~WritableBase() = default;

	virtual WritableBase* Clone() const = 0;
		
	virtual std::ostream& Write(std::ostream& os) const { return os; };

	bool isOwn() const {return _own;}

protected:
	virtual void freePtr() = 0;

	void setPtr(const void* ptr) { _ptr = ptr; }
	const void* getPtr() const { return _ptr; }

private:
	const void* _ptr {nullptr};
	
	// if set to true, 'inner' object lifetime depends on this Printer instance
	bool _own { false };
};

template<typename T> class Writable final : public WritableBase {

public:
	Writable(const T& t, const bool own) : WritableBase((void*)& t, own) {
		if (own) {
			const void* innerObjClone = cloneInnerObj();
			setPtr(innerObjClone);
		}
	}

	virtual ~Writable() {
		if (isOwn()) {
			freePtr();
		}
	}

	const T* getTPtr() const {
		return static_cast<const T*>(getPtr());
	}

	WritableBase* Clone() const override {

		auto* clonedObj = new Writable<T>(*getTPtr(), isOwn());
		return clonedObj;
	}

	virtual void freePtr() override {
		const T* tPtr = getTPtr();
		delete tPtr;
		setPtr(nullptr);
	}

	virtual std::ostream& Write(std::ostream& os) const override {
		const T& tRef = *getTPtr();
		return os << tRef;
	}

protected:
	T* cloneInnerObj() const {
		T* tClonePtr = new T(*getTPtr());
		return tClonePtr;
	}
};

std::ostream& operator<<(std::ostream& os, const WritableBase& w) {
	return w.Write(os);
}

class Printer {
public:

	Printer() :_wb(nullptr) {}

	virtual ~Printer() {
		reset();
	}

	// init from ref, object will be copied, and the copy will be owned by Printer
	template<class T> Printer(const T& t) {
		_wb = new Writable<T>(t, true);
	};

	// init from pointer, object is owned by someone else
	template<class T> Printer(const T* t) {
		_wb = new Writable<T>(*t, false);
	};

	template<class T> Printer(T* t) {
		_wb = new Writable<T>(*t, false);
	};

	Printer& operator=(const Printer& other) {
		reset();
		_wb = other._wb->Clone();
		return *this;
	};

	void print() const {

		if (_wb) {
			const bool own = _wb->isOwn();
			// if owned by someone else, enclose value in []-brackets
			std::cout << (own ? "" : "[") << *_wb << (own ? "" : "]") << "\n";
		}
	}

protected:

	void reset() {
		if (_wb) {
			delete _wb;
			_wb = nullptr;
		}
	}

private:
	WritableBase* _wb;
};


// any class that supports << to stream
class NewStuff {
public:
	NewStuff() = default;
	explicit NewStuff(const int n) : _num(n){}

	int _num{0};
};

std::ostream& operator<<(std::ostream& os, const NewStuff& n) {
	os << "NewStuff..." << (n._num > 0 ? std::to_string(n._num) : std::string());
	return os;
}

int main() {

	Printer printer;

	printer = 42;
	printer.print(); //should print "42" to standard output

	int* value = new int(10);
	printer = value;
	printer.print(); // should print "[10]"

	*value = 20; // the value pointer did not change, changed a value by it
	printer.print(); // should print "[20]"

	float* fvalue = new float(9.81f);
	printer = fvalue;
	printer.print(); // should print "[9.81]"

	*fvalue = 0.2 + 0.3;
	printer.print(); // should print "[0.5]"

	printer = std::string("Hello world");
	printer.print();
	////should print "Hello world"
	
	const NewStuff n;
	printer = n;
	printer.print();

	auto *newStuffPtr = new NewStuff(1);
	printer = newStuffPtr;
	printer.print();
	newStuffPtr->_num = 2;
	printer.print();

	printer = 2.718281;
	printer.print();

	delete value;
	delete fvalue;
	delete newStuffPtr;

	return 0;
}