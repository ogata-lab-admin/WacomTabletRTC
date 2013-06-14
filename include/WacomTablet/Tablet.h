#pragma

#include <stdint.h>
#include <string>
#include <exception>

/**
 * タブレット用の例外クラス
 */
class TabletException : public std::exception {
private:
	std::string msg;

public:

	/**
	 * コンストラクタ
	 */
	TabletException(const char* message) : msg(message) {}

	/**
	 * デストラクタ
	 */
	virtual ~TabletException() throw() {}

	/**
	 * メッセージ
	 */
	const char* what() const throw() {return msg.c_str();}
};

struct TabletPoint {
	int32_t x;
	int32_t y;
};

struct TabletOrientation {
	int32_t orAzimuth;
	int32_t orAltitude;
	int32_t orTwist;
};

/**
 *
 */
class Tablet {

	
public:
	virtual bool updateData(void) = 0;

	virtual TabletPoint getPosition() = 0;

	virtual uint32_t getPressure() = 0;

	virtual TabletPoint getZAngle() = 0;

	virtual TabletOrientation getOrientation() = 0;

public:

	Tablet() {}


	virtual ~Tablet() {}
};

