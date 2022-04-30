#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace zyx
{
    //不允许拷贝与赋值
	class Noncopyable 
	{
	public:
        Noncopyable() {};
        /**
         * @brief 不允许拷贝
         */
        Noncopyable(const Noncopyable&) = delete;

        /**
         * @brief 不允许赋值
         */
        Noncopyable& operator=(const Noncopyable&) = delete;
	};
}
#endif // NONCOPYABLE_H

