// Copyright (C) 2023 Toitware ApS.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef GROW_PARTITION_H
#define GROW_PARTITION_H

#include <esp_err.h>
#include <esp_partition.h>

class GrowPartition {
public:
    GrowPartition() = default;
    bool grow();

private:
    static constexpr int RETRY_LIMIT = 10;
    static constexpr int RETRY_WAIT = 100;
    static constexpr size_t PARTITION_TABLE_ADDRESS = 0x8000;
    static constexpr size_t PARTITION_TABLE_SIZE = 0xC00;
    static constexpr size_t PARTITION_TABLE_ALIGNED_SIZE = 0x1000;  // Must be divisible by 4k.

    esp_err_t replace_partition_table();
    bool running_from_ota1();
    bool invalid_or_already_written();
};

#endif // GROW_PARTITION_H