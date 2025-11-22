#!/bin/bash

echo -n 'APPLE' | build/Debug/grep -E '\w' # 0
if [ $? -ne 0 ]; then
  echo "test failed - APPLE"
fi

echo -n "colour" | build/Debug/grep -E "colou?r" # 0
if [ $? -ne 0 ]; then
  echo "test failed - colour"
fi

echo -n 'apple' | build/Debug/grep -E '[^abc]' # 0
if [ $? -ne 0 ]; then
  echo "test failed - apple"
fi

echo -n 'banana' | build/Debug/grep -E '[^anb]' # 1
if [ $? -ne 1 ]; then
  echo "test failed - banana"
fi

echo -n 'blueberry' | build/Debug/grep -E '[acdfghijk]' # 1
if [ $? -ne 1 ]; then
  echo "test failed - blueberry"
fi

echo -n '[]' | build/Debug/grep -E '[orange]' # 1
if [ $? -ne 1 ]; then
  echo "test failed - []"
fi

echo -n "orangeq\\" | build/Debug/grep -E "[^opq]q\\\\" # 0
if [ $? -ne 0 ]; then
  echo "test failed - orangeq\\"
fi

echo -n 'orange_pear' | build/Debug/grep -E '^orange' # 0
if [ $? -ne 0 ]; then
  echo "test failed - orange_pear"
fi

echo -n 'pear_orange' | build/Debug/grep -E '^orange' # 1
if [ $? -ne 1 ]; then
  echo "test failed - pear_orange"
fi

echo -n "helloa123" | build/Debug/grep -E "a\d+" # 0
if [ $? -ne 0 ]; then
  echo "test failed - helloa123"
fi

echo -n 'sally has 3 dogs' | build/Debug/grep -E '\d \w\w\ws' # 0
if [ $? -ne 0 ]; then
  echo "test failed - sally has 3 dogs"
fi

echo -n 'sally has 1 dog' | build/Debug/grep -E '\d \w\w\ws' # 1
if [ $? -ne 1 ]; then
  echo "test failed - sally has 1 dog"
fi

echo -n "a123123123123" | build/Debug/grep -E "a[123]+123" # 0
if [ $? -ne 0 ]; then
  echo "test failed - a123123123123"
fi

echo -n "aaaxbbbacy" | build/Debug/grep -E "a123$" # 1
if [ $? -ne 1 ]; then
  echo "test failed - aaaxbbbacy"
fi

echo -n 'pineapple_pear' | build/Debug/grep -E 'pear$' # 0
if [ $? -ne 0 ]; then
  echo "test failed - pineapple_pear"
fi

echo -n 'pear_pineapple' | build/Debug/grep -E 'pear$' # 1
if [ $? -ne 1 ]; then
  echo "test failed - pear_pineapple"
fi

echo -n 'banana_banana' | build/Debug/grep -E '^banana$' # 1
if [ $? -ne 1 ]; then
  echo "test failed - banana_banana"
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

echo -n 'cat' | build/Debug/grep -E 'ca+t' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat"
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

echo -n 'cat' | build/Debug/grep -E 'ca?t' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat"
fi

echo -n 'act' | build/Debug/grep -E 'ca?t' # 0
if [ $? -ne 0 ]; then
  echo "test failed - act"
fi

echo -n 'cat' | build/Debug/grep -E 'ca?a?t' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat (2)"
fi

echo -n 'cag' | build/Debug/grep -E 'ca?t' # 1
if [ $? -ne 1 ]; then
  echo "test failed - cag"
fi

echo -n "I like fish" | build/Debug/grep -E "I like (cats|dogs)" # 1
if [ $? -ne 1 ]; then
  echo "test failed - I like fish"
fi

echo -n "I like catsdogscatsdogs" | build/Debug/grep -E "I like (cats|dogs)+" # 0
if [ $? -ne 0 ]; then
  echo "test failed - I like catsdogscatsdogs"
fi

echo -n "I like  and parrots" | build/Debug/grep -E "I like (cats|dogs)? and parrots" # 0
if [ $? -ne 0 ]; then
  echo "test failed - I like  and parrots"
fi

echo -n "blue" | build/Debug/grep -E "(red|blue|green)" # 0
if [ $? -ne 0 ]; then
  echo "test failed - blue"
fi

echo -n "doghouse" | build/Debug/grep -E "(cat|dog)" # 0
if [ $? -ne 0 ]; then
  echo "test failed - doghouse"
fi

echo -n "a cog" | build/Debug/grep -E "a (cat|dog)" # 1
if [ $? -ne 1 ]; then
  echo "test failed - a cog"
fi

echo -n "I see 1 cat" | build/Debug/grep -E "^I see \d+ (cat|dog)s?$" # 0
if [ $? -ne 0 ]; then
  echo "test failed - I see 1 cat"
fi

echo -n "I see 2 dog3" | build/Debug/grep -E "^I see \d+ (cat|dog)s?$" # 1
if [ $? -ne 1 ]; then
  echo "test failed - I see 2 dog3"
fi

echo -n "goøö0Ogol" | build/Debug/grep -E "g.+gol" # 0
if [ $? -ne 0 ]; then
  echo "test failed - goøö0Ogol"
fi

echo -n "car" | build/Debug/grep -E "c.t" # 1
if [ $? -ne 1 ]; then
  echo "test failed - car"
fi

echo -n "cat" | build/Debug/grep -E "c.t" # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat"
fi

echo -n 'gol' | build/Debug/grep -E 'g.+gol' # 1
if [ $? -ne 1 ]; then
  echo "test failed - gol"
fi

echo -n 'cat and cat' | build/Debug/grep -E '(\w+) and \1' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat and cat."
fi

echo -n 'cat and dog' | build/Debug/grep -E '(\w+) and \1' # 1
if [ $? -ne 1 ]; then
  echo "test failed - cat and dog ('(\w+) and \1')."
fi

echo -n 'cat and dog' | build/Debug/grep -E '(cat) and \1' # 1
if [ $? -ne 1 ]; then
  echo "test failed - cat and dog ('(cat) and \1')."
fi

echo -n '123-123' | build/Debug/grep -E '(\d+)-\1' # 0
if [ $? -ne 0 ]; then
  echo "test failed - 123-123."
fi

echo -n 'cat is cat, not dog' | build/Debug/grep -E '^([act]+) is \1, not [^xyz]+$' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat is cat, not dog."
fi

echo -n '3 red and 3 red' | build/Debug/grep -E '(\d+) (\w+) and \1 \2' # 0
if [ $? -ne 0 ]; then
  echo "test failed - 3 red and 3 red."
fi

echo -n '3 red and 4 red' | build/Debug/grep -E '(\d+) (\w+) and \1 \2' # 1
if [ $? -ne 1 ]; then
  echo "test failed - 3 red and 4 red."
fi

echo -n 'cat and dog are dog and cat' | build/Debug/grep -E '(cat) and (dog) are \2 and \1' # 0
if [ $? -ne 0 ]; then
  echo "test failed - cat and dog are dog and cat."
fi

# nesting not currently supported
# echo -n 'somethinggoodbye' | build/Debug/grep -E '(something(hello|goodbye))' # 0
# if [ $? -ne 0 ]; then
#   echo "test failed - somethinggoodbye."
# fi
