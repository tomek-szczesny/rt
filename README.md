# rt

A resistance matching tool for electronic engineers.

This tool is the quickest way to find:

- Resistor networks approximating the input value

- Sets of resistors with defined ratios between each other.

The tool works with prebuilt resistor values lists and accepts custom value lists, representing your personal, company or supplier's stock.

## Screenshots

![2024-12-21-213726_654x545_scrot](https://github.com/user-attachments/assets/e8807fe7-acb8-446c-bb08-bf79df493f50)

![2024-12-21-213701_714x553_scrot](https://github.com/user-attachments/assets/1b5e5dc9-3691-4f37-a37a-01424a1c2db5)


## Installation

```
git clone https://github.com/tomek-szczesny/rt.git
cd rt
make
sudo make install
```

## Usage

```
rt [options] [function] value0 value1 value2...

options:

	-l [name]  Use a list of values from 'name'. (unspecified uses 'default')
		   Search precedence:
		     /Absolute/Path/name
		     ./lists/name
		     ~/.config/rt/lists/name
		     /var/lib/rt/lists/name

function: (one function only)

	-c	  Find a combination of resistors approximating a single given value. (default)

	-r	  Look for a set of resistors satisfying a given ratio between them.
		  If only one value is given analyze for the ratio value:1 otherwise,
		  additional values are interpreted as weights of each resistor.
		  Error is the product of the maximum and minimum resistance/weight ratio, minus one.

Examples:
		  Find the best approximation of 12.34k resistance:
		  rt 12.34k

		  Find the best approximation of 12.34k resistance in E24 series:
		  rt -l e24 12.34k

		  Find the best set of resistors to build a 5-bit DAC resistor ladder:
		  rt -r 1 2 4 8 16
```
