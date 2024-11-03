 # this file should print the following (only the numbers):
# 9
# 16
 # 0
 # 2
 # 2.500000
 # 3
 # 1
 #
 #
 #
 # additional whitespace except at the start of a line is valid (? still waiting on clarification ?)
 # 9 is printed
 one  <-    1
 #
 function  increment   value
 	return    value   +   one
 #
 print   increment(  3 )  +  increment  ( 4  )
 #
 #
 #
 # from [help2002]⬈
 # functions can call another function as a parameter
 # 16 is printed

# 18 is printed
#
function printsum a b
	print a + b
#
printsum (12, 6)

 function printsum a b
 	print a + b
 printsum(increment(5) ,  10)
 #
 #
 #
 # variables do not need to be defined before being used in an expression, and are automatically initialised to the (real) value 0.0
 print d
 #
 #
 #
 $ from [help2002]⬈ 
 # functions do not need to end with a return or print statement, if there is no tab the function has ended
 # variable reassignment is allowed
 # 2 is printed
 function none   
 	one -> 2
 none(   )
 print one
 #
 #
 #
 # comments can start in the middle of a line
 # 2.500000 is printed
 print 2.5 # comment
 #
 #
 #
 # from [help2002]⬈
 # 3 is printed then 1 is printed, since noreturn(1, 2) would return 0
 function noreturn x y
     print x + y
 print 1 + noreturn(1, 2)
 