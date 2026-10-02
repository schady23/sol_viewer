#include "cli.hpp"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
template<class F> void rejects(F f) {
    bool rejected=false; try { f(); } catch(const std::exception&) { rejected=true; }
    check(rejected,"Malformed numeric input accepted");
}
int main() {
    try {
        check(solar::cli::integer("2147483647")==2147483647,"Integer upper bound");
        check(solar::cli::integer("-2147483648")==(-2147483647-1),"Integer lower bound");
        check(solar::cli::real("1.25")==1.25f,"Fractional real input");
        check(solar::cli::number("1e2")==100,"Scientific notation");
        for(const char* value:{"", "2junk", "2.5", "1e2", "2147483648", "-2147483649", "NaN", "inf"})
            rejects([&] { solar::cli::integer(value); });
        for(const char* value:{"", "1.5junk", "NaN", "inf", "-inf", "1e999"})
            rejects([&] { solar::cli::number(value); });
        rejects([] { solar::cli::real("1e300"); });
        std::cout<<"PASS: strict finite numbers, integer bounds and float overflow\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
