#!/bin/bash

echo -n "colour" | build/Debug/grep -E "colou?r" # 0
if [ $? -ne 0 ]; then
  echo "test failed - colour"
fi

echo -n "orangeq\\" | build/Debug/grep -E "[^opq]q\\\\" # 0
if [ $? -ne 0 ]; then
  echo "test failed - orangeq\\"
fi

echo -n "helloa123" | build/Debug/grep -E "a\d+" # 0
if [ $? -ne 0 ]; then
  echo "test failed - helloa123"
fi

echo -n "a123123123123" | build/Debug/grep -E "a[123]+123" # 0
if [ $? -ne 0 ]; then
  echo "test failed - a123123123123"
fi

echo -n "aaaxbbbacy" | build/Debug/grep -E "a123$" # 1
if [ $? -ne 1 ]; then
  echo "test failed - aaaxbbbacy"
fi

echo -n "abcthisisabc" | build/Debug/grep -E "^abc" # 0
if [ $? -ne 0 ]; then
  echo "test failed - abcthisisabc"
fi

echo -n "thisisajvm" | build/Debug/grep -E "^[jmav]+" # 1
if [ $? -ne 1 ]; then
  echo "test failed - thisisajvm"
fi

echo -n "thisisnotthis" | build/Debug/grep -E "this$" # 0
if [ $? -ne 0 ]; then
  echo "test failed - thisisnotthis"
fi

echo -n "caaars" | build/Debug/grep -E "ca+aars" # 0
if [ $? -ne 0 ]; then
  echo "test failed - caaars"
fi

echo -n "dog" | build/Debug/grep -E "d" # 0
if [ $? -ne 0 ]; then
  echo "test failed - dog"
fi

echo -n "strawberry" | build/Debug/grep -E "^strawberry$" # 0
if [ $? -ne 0 ]; then
  echo "test failed - strawberry"
fi

echo -n "abc_123_xyz" | build/Debug/grep -E "^abc_\d+_xyz$" # 0
if [ $? -ne 0 ]; then
  echo "test failed - abc_123_xyz"
fi

echo -n "abc_rst_xyz" | build/Debug/grep -E "^abc_\d+_xyz$" # 1
if [ $? -ne 1 ]; then
  echo "test failed - abc_123_xyz"
fi

echo -n "caabts" | build/Debug/grep -E "ca+abt" # 0 
if [ $? -ne 0 ]; then
  echo "test failed - caabts"
fi

echo -n "" | build/Debug/grep -E "\d?" # 0 
if [ $? -ne 0 ]; then
  echo "test failed - \"\""
fi

echo -n "dogs" | build/Debug/grep -E "dogs?" # 0 
if [ $? -ne 0 ]; then
  echo "test failed - dogs"
fi

echo -n "dog" | build/Debug/grep -E "dogs?" # 0 
if [ $? -ne 0 ]; then
  echo "test failed - dog"
fi

echo -n "I like fish" | build/Debug/grep -E "I like (cats|dogs)" # 1
if [ $? -ne 1 ]; then
  echo "test failed - I like fish"
fi

echo -n "blue" | build/Debug/grep -E "(red|blue|green)" # 0
if [ $? -ne 0 ]; then
  echo "test failed - blue"
fi

echo -n "doghouse" | build/Debug/grep -E "(cat|dog)" # 0
if [ $? -ne 0 ]; then
  echo "test failed - doghouse"
fi
