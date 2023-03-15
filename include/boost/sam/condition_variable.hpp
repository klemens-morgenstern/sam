// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SAM_CONDITION_VARIABLE_HPP
#define BOOST_SAM_CONDITION_VARIABLE_HPP
#include <boost/sam/detail/config.hpp>
#include <boost/sam/basic_condition_variable.hpp>

BOOST_SAM_BEGIN_NAMESPACE

/// basic_condition_variable with default executor.
using condition_variable = basic_condition_variable<>;

BOOST_SAM_END_NAMESPACE
#endif //BOOST_SAM_CONDITION_VARIABLE_HPP
