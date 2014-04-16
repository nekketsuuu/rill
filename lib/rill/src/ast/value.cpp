//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>

#include <rill/ast/value.hpp>
#include <rill/ast/expression.hpp>
#include <rill/ast/statement.hpp>

#include <rill/environment/environment.hpp>

#define RILL_AST_MAKE_DEFINITION
#define RILL_AST_FILE_RELOAD
# include <rill/ast/value_def.ipp>
#undef RILL_AST_FILE_RELOAD
#undef RILL_AST_MAKE_DEFINITION


namespace rill
{
    namespace ast
    {
        std::ostream& operator<<( std::ostream& os, value const& vp )
        {
            if ( vp.is_intrinsic() || vp.is_system() ) {
                auto const& iv = static_cast<intrinsic::value_base const&>( vp );

                os << "  type  is " << iv.get_native_typename_string() << std::endl;
                // TODO: add primitive type number
                if ( iv.get_native_typename_string() == "int" ) {
                    os << "  value is " << static_cast<intrinsic::int32_value const&>( iv ).value_ << std::endl;
                } else {
                    os << "  value is unknown." << std::endl;
                }
            } else {
                os << "  NOT typed value." << std::endl;
            }

            return os;
        }

    } // namespace ast
} // namespace rill
