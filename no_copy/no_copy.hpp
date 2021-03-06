// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef NO_COPY_H
#define NO_COPY_H

#include <utility>

namespace qc {

template <typename T>
class no_copy : public T {
public:
    using T::T;
    using T::operator=;

    no_copy(no_copy&& other) = default;
    no_copy(T&& other) : T(std::move(other)) {}

    no_copy& operator=(no_copy&& other) = default;
    no_copy& operator=(T&& other) { return static_cast<no_copy&>(T::operator=(std::move(other))); };

    no_copy clone() const { return no_copy{*this}; }

private:
    // don't delete, we need it to clone
    no_copy(const no_copy&) = default;

    no_copy(const T&) = delete;
    no_copy& operator=(const no_copy&) = delete;
    no_copy& operator=(const T&) = delete;
};

} // namespace qc

#endif // NO_COPY_H
