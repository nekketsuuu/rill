//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RILL_SEMANTIC_ANALYSIS_ANALYZER_IDENTIFIER_FUNCTION_SOLVER_HPP
#define RILL_SEMANTIC_ANALYSIS_ANALYZER_IDENTIFIER_FUNCTION_SOLVER_HPP

#include <boost/range/adaptor/transformed.hpp>

#include "../../environment/environment.hpp"
#include "../../ast/ast.hpp"


namespace rill
{
    namespace semantic_analysis
    {
        using namespace boost::adaptors;// TODO: remove

        static inline auto determine_type_attributes(
            attribute::type_attributes_optional const& attr = attribute::type_attributes_optional()
            )
            -> attribute::type_attributes
        {
            return attribute::make_type_attributes(
                attr.quality
                ? *attr.quality
                : attribute::quality_kind::k_val,                   // default value
                attr.modifiability
                ? *attr.modifiability
                : attribute::modifiability_kind::k_immutable        // defaut value
                );
        }


        template<typename TypeIds, typename EnvPtr, typename ResultCallbackT>
        static auto overload_solver(
            TypeIds const& arg_type_ids,
            std::shared_ptr<has_parameter_environment<function_symbol_environment>> const& generic_function_env,
            EnvPtr const& env,
            ResultCallbackT const& f
            )
            -> function_symbol_environment_ptr
        {
            //
            //
            type_id_list_t holder( arg_type_ids.size() );
            std::vector<function_symbol_environment_ptr> f_candidate_envs;


            // DEBUG
            //std::cout << "...resolving function : " << identifier->get_inner_symbol()->to_native_string() << std::endl;


            // TODO: fix over load solver
            // TODO: check variadic parameter...
            // TODO: count conversion times
            for( auto const& f_env : generic_function_env->get_overloads() ) {
                // DEBUG
                std::cout << "[overloads] " << f_env->mangled_name() << " ..." << std::endl
                          << (const_environment_base_ptr)f_env << std::endl;


                auto const& f_env_parameter_type_ids = f_env->get_parameter_type_ids();

                // argument size is different
                if ( f_env_parameter_type_ids.size() != arg_type_ids.size() )
                    continue;

                // has no argument
                if ( f_env_parameter_type_ids.size() == 0 ) {
                    // TODO: check context...(Ex. pure)
                    std::cout << "~~~0~~~~" << std::endl;
                    f_candidate_envs.push_back( f_env );
                    continue;
                }

                //
                bool succeed = true;
                for( int i=0; i<arg_type_ids.size(); ++i ) {
                    if ( f_env_parameter_type_ids[i] == arg_type_ids[i] ) {
                        // has same type!
                        holder[i] = arg_type_ids[i];

                    } else {
                        // try to type conversion
                        auto const& f_env_arg_type
                            = f_env->get_type_at( f_env_parameter_type_ids[i] );
                        auto self_arg_type = env->get_type_at( arg_type_ids[i] );


                        // 1. try to attribute check and conversion

                        // 1.1 check quarity comversion
                        if ( f_env_arg_type.attributes.quality != self_arg_type.attributes.quality ) {
                            switch( f_env_arg_type.attributes.quality )
                            {
                            case attribute::quality_kind::k_ref:

                                switch( self_arg_type.attributes.quality )
                                {
                                case attribute::quality_kind::k_val:
                                    // val -> ref conversion

                                    if ( f_env_arg_type.attributes.modifiability != self_arg_type.attributes.modifiability ) {

                                        // check modifiability
                                        switch( f_env_arg_type.attributes.modifiability )
                                        {
                                        case attribute::modifiability_kind::k_mutable:

                                            switch( self_arg_type.attributes.modifiability )
                                            {
                                            case attribute::modifiability_kind::k_mutable:
                                                // mutable -> mutable : valid
                                                break;

                                            case attribute::modifiability_kind::k_const:
                                                // mutable -> const : valid
                                                break;

                                            case attribute::modifiability_kind::k_immutable:
                                                // mutable -> immutable : INVARID
                                                // TODO: check flag
                                                assert( false );
                                                break;

                                            default:
                                                assert( false && "[ice]" );
                                                break;
                                            }


                                            break;

                                        case attribute::modifiability_kind::k_const:
                                            // TODO: implement
                                            assert( false );
                                            break;

                                        case attribute::modifiability_kind::k_immutable:
                                            // TODO: implement
                                            //assert( false );
                                            break;

                                        default:
                                            assert( false && "[ice]" );
                                            break;
                                        }


                                        // copy modifiablity
                                        self_arg_type.attributes <<= f_env_arg_type.attributes.modifiability;
                                    }

                                    break;

                                default:
                                    // TODO: implement
                                    assert( false && "[ice]" );
                                    break;
                                }





                                break;

                            case attribute::quality_kind::k_val:
                                // All type -> val is convertible at the moment
                                self_arg_type = f_env_arg_type; //attribute::quality_kind:k_val;

                                break;

                            default:
                                // TODO: implement
                                assert( false && "[ice]" );
                                break;
                            }
                        }
                        if ( !succeed )
                            break;   // change overload resolution target



                        // TODO: remove this
                        self_arg_type.attributes <<= f_env_arg_type.attributes.modifiability;




                        // 2. class type conversion
                        if ( f_env_arg_type.class_env_id != self_arg_type.class_env_id ) {
                            // TODO: implement
                            //assert( false && "[ice]" );
                            succeed = false;
                        }
                        if ( !succeed )
                            break;   // change overload resolution target

                        // rewrite
                        holder[i] = env->make_type_id( f_env_arg_type.class_env_id, f_env_arg_type.attributes );
                    }
                } // for
                if ( succeed ) {
                    f_candidate_envs.push_back( f_env );
                }
            }


            return f( f_candidate_envs );
        }




        template<typename TypeIds, typename EnvPtr>
        static inline auto overload_solver(
            TypeIds const& arg_type_ids ,
            std::shared_ptr<has_parameter_environment<function_symbol_environment>> const& generic_function_env,
            EnvPtr const& env
            )
            -> function_symbol_environment_ptr
        {
            return overload_solver(
                arg_type_ids,
                generic_function_env,
                env,
                []( std::vector<function_symbol_environment_ptr> const& f_candidate_envs ) {

                    size_t selected = 0;
                    if ( f_candidate_envs.size() == 0 ) {
                        // TODO: to search other namespaces...
                        assert( false && "Overload failed... [not implemented]");
                    } else if ( f_candidate_envs.size() > 1 ) {
                        // TODO: check comversion times...
                        assert( false && "duplecate?... [not implemented]");
                    }

                    return f_candidate_envs[selected];
                }
                );
        }

        template<typename TypeIds, typename EnvPtr>
        static inline auto overload_solver_allow_no_entry(
            TypeIds const& arg_type_ids ,
            std::shared_ptr<has_parameter_environment<function_symbol_environment>> const& generic_function_env,
            EnvPtr const& env
            )
            -> function_symbol_environment_ptr
        {
            return overload_solver(
                arg_type_ids,
                generic_function_env,
                env,
                []( std::vector<function_symbol_environment_ptr> const& f_candidate_envs )-> function_symbol_environment_ptr
                {
                    size_t selected = 0;
                    if ( f_candidate_envs.size() == 0 ) {
                        return nullptr;
                    } else if ( f_candidate_envs.size() > 1 ) {
                        // TODO: check comversion times...
                        assert( false && "duplecate?... [not implemented]");
                    }

                    return f_candidate_envs[selected];
                }
                );
        }






        struct to_type_id_t2
        {
            typedef type_id_t result_type;

            template<typename T>
            auto operator()(T const& c) const
                -> result_type
            {
                return c->type_id;
            }
        };








        template<typename Visitor,
                 typename TemplateArgs,
                 typename TypeIds,
                 typename EnvPtr,
                 typename ResultCallbackT
                 >
        auto overload_solver_with_template(
            Visitor visitor,
            TemplateArgs const& template_args,
            TypeIds const& arg_type_ids2,
            std::shared_ptr<template_set_environment> const& template_set_env,
            EnvPtr const& env,
            ResultCallbackT const& f
            )
            -> function_symbol_environment_ptr
        {
            assert( template_args != nullptr );

            // TODO: add duplication check

            std::shared_ptr<has_parameter_environment<function_symbol_environment>> generic_function_env
                = nullptr;


            //
            for( auto const& template_env : template_set_env->get_candidates() ) {
                // TODO: add template length check...

                std::cout << "hogehoge !" << std::endl;

                // if number of template arguments is over, skip
                if ( template_args->size() > template_env->get_parameter_num() )
                    continue;



                auto const& template_ast
                    = std::static_pointer_cast<ast::template_statement>( template_env->get_related_ast() );
                assert( template_ast != nullptr );




                // ==================================================
                // INNER function
                // ==================================================

                // make new ast(cloned)
                auto const& function_ast
                    = std::static_pointer_cast<ast::function_definition_statement>(
                        template_ast->clone_inner_node()
                        );
                assert( function_ast != nullptr );


                // Create function emvironment frame
                // FIX: template_env to another
                auto f_env_pair
                    = template_set_env->get_parent_env()->incomplete_construct(
                        kind::k_function,
                        function_ast->get_identifier()
                        );

                generic_function_env = f_env_pair.first;
                auto f_env = f_env_pair.second;


                std::cout << "fugafuga" << std::endl;

                std::cout << "TEMPLATE bef" << std::endl;

                // template parameters
                // import template parameter's variables with instantiation!
                visitor->tp(
                    template_ast->get_parameter_list(),
                    template_args,
                    f_env,
                    /*parent_env*/env
                    );


                std::cout << "TEMPLATE aftre" << (const_environment_base_ptr)f_env << std::endl;




                //
                // function instanciation
                // TODO: generize

                // make function parameter variable decl
                for( auto const& e : function_ast->get_parameter_list() ) {
                    assert( e.decl_unit.init_unit.type != nullptr || e.decl_unit.init_unit.initializer != nullptr );

                    if ( e.decl_unit.init_unit.type ) { // is parameter variavle type specified
                        solve_type(
                            visitor,
                            e.decl_unit.init_unit.type,
                            /*parent_env*/f_env,
                            [&]( type_detail_ptr const& ty_d,
                                 type const& ty,
                                 class_symbol_environment_ptr const& class_env
                                ) {
                                    auto attr = ty.attributes;
                                    attr <<= e.quality;

                                    std::cout << "<<TEMPLATE PARAM CLASS>> : "
                                              << class_env->get_qualified_name() << std::endl;

                                    // declare
                                    f_env->parameter_variable_construct(
                                        e.decl_unit.name,
                                        class_env,
                                        attr
                                        );
                            });

                    } else {
                        // type inferenced by result of evaluated [[default initializer expression]]

                        // TODO: implement type inference
                        assert( false );
                    }
                }



                // scan all statements in this function body
                visitor->dispatch( function_ast->inner_, f_env );


                // ?: TODO: use block expression


                // Return type
                if ( function_ast->return_type_ ) {
                    solve_type(
                        visitor,
                        *function_ast->return_type_,
                        /*parent_env*/f_env,
                        [&]( type_detail_ptr const& return_ty_d,
                             type const& ty,
                             class_symbol_environment_ptr const& class_env
                            ) {

                            // mangling
                            std::string mangle;






                            f_env->complete(
                                return_ty_d->type_id,
                                function_ast->get_identifier()->get_inner_symbol()->to_native_string(),
                                std::to_string( std::rand() )/*FIXME*/
                                );
                        });

                } else {
                    // TODO: implement return type inference
                    assert( false && "function return type inference was not supported yet" );
                }

                //
                std::cout << std::endl << "!!add overload!!" << std::endl << std::endl;
                f_env->get_parameter_wrapper_env()->add_overload( f_env );


                // ???
                f_env->link_with_ast( function_ast );
            } // for





            assert( generic_function_env != nullptr );


            std::cout
                << (const_environment_base_ptr)generic_function_env << std::endl
                << "overload num is " << generic_function_env->get_overloads().size() << std::endl;

            // DEBUG
            {
                auto const& arg_type_ids = arg_type_ids2 | transformed( to_type_id_t2() );

                return overload_solver(
                    arg_type_ids,
                    generic_function_env,
                    env,
                    f
                    );
            }
        }




        template<typename Visitor,
                 typename TemplateArgs,
                 typename TypeIds,
                 typename EnvPtr
                 >
        static inline auto overload_solver_with_template(
            Visitor visitor,
            TemplateArgs const& template_args,
            TypeIds const& arg_type_ids,
            std::shared_ptr<template_set_environment> const& generic_function_env,
            EnvPtr const& env
            )
            -> function_symbol_environment_ptr
        {
            return overload_solver_with_template(
                visitor,
                template_args,
                arg_type_ids,
                generic_function_env,
                env,
                []( std::vector<function_symbol_environment_ptr> const& f_candidate_envs ) {

                    size_t selected = 0;
                    if ( f_candidate_envs.size() == 0 ) {
                        // TODO: to search other namespaces...
                        assert( false && "Overload failed... [not implemented]");
                    } else if ( f_candidate_envs.size() > 1 ) {
                        // TODO: check comversion times...
                        assert( false && "duplecate?... [not implemented]");
                    }

                    return f_candidate_envs[selected];
                }
                );
        }

        template<typename AnalyzerPtr,
                 typename TemplateArgs,
                 typename TypeIds,
                 typename EnvPtr
                 >
        static inline auto overload_solver_allow_no_entry_with_template(
            AnalyzerPtr visitor,
            TemplateArgs const& template_args,
            TypeIds const& arg_type_ids ,
            std::shared_ptr<template_set_environment> const& generic_function_env,
            EnvPtr const& env
            )
            -> function_symbol_environment_ptr
        {
            return overload_solver_with_template(
                visitor,
                template_args,
                arg_type_ids,
                generic_function_env,
                env,
                []( std::vector<function_symbol_environment_ptr> const& f_candidate_envs )-> function_symbol_environment_ptr
                {
                    size_t selected = 0;
                    if ( f_candidate_envs.size() == 0 ) {
                        return nullptr;
                    } else if ( f_candidate_envs.size() > 1 ) {
                        // TODO: check comversion times...
                        assert( false && "duplecate?... [not implemented]");
                    }

                    return f_candidate_envs[selected];
                }
                );
        }


    } // namespace semantic_analysis
} // namespace rill

#endif /*RILL_SEMANTIC_ANALYSIS_ANALYZER_IDENTIFIER_FUNCTION_SOLVER_HPP*/