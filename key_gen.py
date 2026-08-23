import sys
import sqlite3
import datetime
from server import create_license_key, init_db

def main():
    init_db()
    print("========================================")
    print("       LICENSE KEY GENERATOR            ")
    print("========================================")
    
    if len(sys.argv) > 1:
        try:
            days = int(sys.argv[1])
        except ValueError:
            print("Usage: python key_gen.py [days]")
            print("Example: python key_gen.py 30")
            sys.argv = []

    if len(sys.argv) <= 1:
        print("Select key duration:")
        print("1. 1 Day (Trial)")
        print("2. 7 Days (Weekly)")
        print("3. 30 Days (Monthly)")
        print("4. 365 Days (Yearly)")
        print("5. Lifetime (0 days)")
        print("6. Custom Days")
        choice = input("Enter choice (1-6) [default: 3]: ").strip() or "3"

        if choice == "1":
            days = 1
        elif choice == "2":
            days = 7
        elif choice == "3":
            days = 30
        elif choice == "4":
            days = 365
        elif choice == "5":
            days = 0
        elif choice == "6":
            days = int(input("Enter custom days: ").strip())
        else:
            days = 30

    key = create_license_key(days)
    label = "Lifetime" if days == 0 else f"{days} Days"
    print("\n----------------------------------------")
    print(f" GENERATED LICENSE KEY ({label}):")
    print(f" >>  {key}  <<")
    print("----------------------------------------")
    print("Give this key to your customer!")

if __name__ == "__main__":
    main()
