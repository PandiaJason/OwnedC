#include "ownedc.hpp"
#include <iostream>

struct Hero {
    int hp;
    int mp;
    
    Hero() : hp(100), mp(50) {
        std::cout << "Hero constructed!" << std::endl;
    }
    
    ~Hero() {
        std::cout << "Hero destructed!" << std::endl;
    }
};

void battle(ownedc::borrowed_ptr<Hero> h) {
    std::cout << "Hero HP in battle: " << h->hp << std::endl;
    h->hp -= 10;
}

int main() {
    std::cout << "--- OwnedC C++ RAII Demo ---" << std::endl;
    
    // Create an owned pointer using our C++ wrapper. This will automatically call the Hero constructor.
    auto hero = ownedc::owned_ptr<Hero>::make();
    
    // Borrow it safely
    {
        ownedc::borrowed_ptr<Hero> borrowed_hero(hero);
        battle(borrowed_hero);
    } // Borrow is released here safely
    
    std::cout << "Hero HP after battle: " << hero->hp << std::endl;
    
    // The hero's memory and destructor are automatically managed here!
    std::cout << "Demo finished successfully." << std::endl;
    return 0;
}
