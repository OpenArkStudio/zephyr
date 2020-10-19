/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 * 
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ZEPHYR_VERSION_HPP__
#define __ZEPHYR_VERSION_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


/** @def ZEPHYR_API_VERSION

    Identifies the API version of zephyr.

    This is a simple integer that is incremented by one every
    time a set of code changes is merged to the develop branch.
*/

// asio2 2.5

// ZEPHYR_VERSION / 100 is the major version
// ZEPHYR_VERSION % 100 is the minor version
#define ZEPHYR_VER_MAJOR 1
#define ZEPHYR_VER_MINOR 0
#define ZEPHYR_VER_PATCH 1

#define ZEPHYR_VERSION (ZEPHYR_VER_MAJOR * 10000 + ZEPHYR_VER_MINOR * 100 + ZEPHYR_VER_PATCH)



#endif // !__ZEPHYR_VERSION_HPP__
