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
rt [options] [function] num0 [num1] [num2] ...

Options:

-l [name]    Uses a list of available values from a file 'name'.

The list file is looked up in the following locations (in this order):
(full path to a file)
./lists/name
$HOME/.config/rt/lists/name
/var/lib/rt/lists/name
If unspecified, a list 'default' is used.

Functions:
    Only one function can be specified.
    -c    Finds a combination of resistors approximating the given value.
          Only one numerical argument is anticipated.
          This is the default function.

    -r    Looks for a set of resistors satisfying a given ratio between them.
          If one numerical argument is given, the tool will look for a ratio num:1.
          If more values are provided, the tool will interpret them as weighs of
          each resistor in the set.

Examples:
    Find the best approximation of 12.34k resistance:
    rt 12.34k

    Find the best approximation of 12.34k resistance in E12 series:
    rt -l e12 12.34k

    Find the best set of resistors to build a 5-bit DAC resistor ladder:
    rt -r 1 2 4 8 16
```
