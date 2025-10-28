#!/bin/bash

echo -n "colour" | build/Debug/grep -E "colou?r"
echo -n "orangeq\\" | build/Debug/grep -E "[^opq]q\\\\"
echo -n "helloa123" | build/Debug/grep -E "a\\d+"
echo -n "a123123123123" | build/Debug/grep -E "a[123]+123"
echo -n "aaaxbbbacy" | build/Debug/grep -E "a123$"
echo -n "abcthisisabc" | build/Debug/grep -E "^abc"
echo -n "thisisajvm" | build/Debug/grep -E "^[jmav]+"
echo -n "thisisnotthis" | build/Debug/grep -E "this$"
echo -n "caaars" | build/Debug/grep -E "ca+aars"
echo -n "dog" | build/Debug/grep -E "d"
echo -n "strawberry" | build/Debug/grep -E "^strawberry$"
echo -n "abc_123_xyz" | build/Debug/grep -E "^abc_\\d+_xyz$"
echo -n "abc_rst_xyz" | build/Debug/grep -E "^abc_\\d+_xyz$"
echo -n "caabts" | build/Debug/grep -E "ca+abt" # 0 
echo -n "" | build/Debug/grep -E "\d?" # 0 
echo -n "dogs" | build/Debug/grep -E "dogs?" # 0 
echo -n "dog" | build/Debug/grep -E "dogs?" # 0 
