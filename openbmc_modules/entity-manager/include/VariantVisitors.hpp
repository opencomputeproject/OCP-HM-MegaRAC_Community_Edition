/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <stdexcept>
#include <string>
#include <variant>

struct VariantToFloatVisitor
{

    template <typename T>
    float operator()(const T& t) const
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return static_cast<float>(t);
        }
        throw std::invalid_argument("Cannot translate type to float");
    }
};

struct VariantToIntVisitor
{
    template <typename T>
    int operator()(const T& t) const
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return static_cast<int>(t);
        }
        throw std::invalid_argument("Cannot translate type to int");
    }
};

struct VariantToUnsignedIntVisitor
{
    template <typename T>
    unsigned int operator()(const T& t) const
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return static_cast<unsigned int>(t);
        }
        throw std::invalid_argument("Cannot translate type to unsigned int");
    }
};

struct VariantToStringVisitor
{
    template <typename T>
    std::string operator()(const T& t) const
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return t;
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            return std::to_string(t);
        }
        throw std::invalid_argument("Cannot translate type to string");
    }
};

struct VariantToDoubleVisitor
{
    template <typename T>
    double operator()(const T& t) const
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return static_cast<double>(t);
        }
        throw std::invalid_argument("Cannot translate type to double");
    }
};
