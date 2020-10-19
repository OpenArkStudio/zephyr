/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 *
 * refrenced from https://github.com/loveyacper/ananas
 */

#ifndef __ZEPHYR_DEFER_HPP__
#define __ZEPHYR_DEFER_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <functional>

namespace zephyr
{

	class defer
	{
	public:
		defer() = default;

		// movable
		defer(defer&&) = default;
		defer& operator=(defer&&) = default;

		// non copyable
		defer(const defer&) = delete;
		void operator=(const defer&) = delete;

		template <typename Fun, typename... Args>
		defer(Fun&& fun, Args&&... args)
		{
			this->fn_ = std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...);
		}

		~defer() noexcept
		{
			if (this->fn_) this->fn_();
		}

	protected:
		std::function<void()> fn_;
	};

#define ZEPHYR_CONCAT(a, b) a##b
#define ZEPHYR_MAKE_DEFER(line) ::zephyr::defer ZEPHYR_CONCAT(_zephyr_defer_, line) = [&]()
#define ZEPHYR_DEFER ZEPHYR_MAKE_DEFER(__LINE__)

}

#endif  // !__ZEPHYR_DEFER_HPP__
