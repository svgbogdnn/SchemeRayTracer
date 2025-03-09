#include "lambda.hpp"

namespace Interp {

ObjectPtr BuildLambda(std::optional<std::string> name, ObjectPtr init, ScopePtr scope) {
    std::vector<ObjectPtr> arguments = Interp::ListToVector(init);

    std::vector<ObjectPtr> commands{};
    std::vector<std::string> arguments_idx_to_name{};

    if (arguments.size() < 2) {
        throw SyntaxError("More than 1 argument required for \"BuildLambda\" function");
    }

    // Handle lambda arguments
    if (arguments[0] && !Is<Cell>(arguments[0])) {
        throw RuntimeError("\"BuildLambda\" error: first argument (lambda arguments) is not a list");
    }

    std::shared_ptr<Cell> lambda_arg_init = As<Cell>(arguments[0]);
    for (std::shared_ptr<Cell> cell = As<Cell>(lambda_arg_init); cell; cell = As<Cell>(cell->GetSecond())) {
        if (!Is<Symbol>(cell->GetFirst())) {
            throw RuntimeError("\"BuildLambda\" error: first argument (lambda arguments): not a symbol met");
        }
        std::string argument_name = As<Symbol>(cell->GetFirst())->GetName();
        arguments_idx_to_name.push_back(argument_name);
    }

    // Handle lambda body
    if (!Is<Cell>(arguments[1])) {
        throw RuntimeError("\"BuildLambda\" error: second argument (body) is not a list");
    }

    for (number_t argument_idx = 1; argument_idx < arguments.size(); ++argument_idx) {
        commands.push_back(arguments[argument_idx]);
    }
    
    ScopePtr lambda_self_scope = std::make_shared<Scope>();
    return std::make_shared<LambdaInterp>(commands, arguments_idx_to_name, lambda_self_scope);
}

std::pair<std::string, ObjectPtr> BuildLambdaSugar(std::vector<ObjectPtr> parts, ScopePtr scope) {
    if (parts.size() != 2) {
        throw SyntaxError("Exactly 2 arguments required for \"BuildLambdaSugar\" function");
    }

    std::vector<ObjectPtr> arguments = Interp::ListToVector(parts[0]);
    ObjectPtr command = parts[1];

    std::vector<ObjectPtr> commands{};
    std::vector<std::string> arguments_idx_to_name{};

    // Handle lambda arguments
    std::string lambda_name = As<Symbol>(arguments[0])->GetName();
    for (size_t argument_idx = 1; argument_idx < arguments.size(); ++argument_idx) {
        if (!Is<Symbol>(arguments[argument_idx])) {
            throw SyntaxError("\"BuildLambdaSugar\" function error: argument passed is not a symbol");
        }
        std::string argument_name = As<Symbol>(arguments[argument_idx])->GetName();
        arguments_idx_to_name.push_back(argument_name);
    }

    // Handle lambda body
    commands.push_back(command);

    ScopePtr lambda_self_scope = std::make_shared<Scope>();
    return std::make_pair(lambda_name, std::make_shared<LambdaInterp>(commands, arguments_idx_to_name, lambda_self_scope));
}

} // namespace Interp

namespace Codegen {

ObjectPtr BuildLambdaCodegen(std::optional<std::string> name, ObjectPtr init, ScopePtr scope) {
    auto& context = Codegen::Context::Get();

    std::vector<ObjectPtr> arguments = Interp::ListToVector(init);
    ScopePtr lambda_call_and_self_scope = std::make_shared<Scope>();
    lambda_call_and_self_scope->SetPreviousScope(scope);

    // LLVM CTORS
    int temp_argument_counter = 0; // TODO: fix
    std::shared_ptr<Cell> lambda_arg_init = As<Cell>(arguments[0]);
    for (std::shared_ptr<Cell> cell = As<Cell>(lambda_arg_init); cell; cell = As<Cell>(cell->GetSecond())) {
        ++temp_argument_counter;
    }
    int arguments_count = temp_argument_counter;
    std::vector<llvm::Type*> new_function_arguments(arguments_count, context.object_ptr_type);
    llvm::FunctionType* new_function_type = llvm::FunctionType::get(context.object_type, new_function_arguments, false);
    llvm::Function* new_function = llvm::Function::Create(new_function_type, llvm::Function::ExternalLinkage, "LambdaFunction", context.llvm_module.value());
    assert(new_function->empty());

    if (name.has_value()) {
        scope->SetVariableFunction(name.value(), std::make_shared<LambdaCodegen>(new_function));
    }
    
    // SET INSERT POINT
    llvm::BasicBlock* old_insert_point = context.builder->GetInsertBlock();
    auto& llvm_context = context.builder->getContext();
    llvm::BasicBlock* new_function_main = llvm::BasicBlock::Create(llvm_context, "entry", new_function);
    context.builder->SetInsertPoint(new_function_main);

    // HANDLE ARGUMENTS
    std::vector<std::string> arguments_idx_to_name{};
    if (arguments.size() < 2) {
        throw SyntaxError("More than 1 argument required for \"BuildLambdaCodegen\" function");
    }

    // Handle lambda arguments
    if (arguments[0] && !Is<Cell>(arguments[0])) {
        throw RuntimeError("\"BuildLambdaCodegen\" error: first argument (lambda arguments) is not a list");
    }

    // std::shared_ptr<Cell> lambda_arg_init = As<Cell>(arguments[0]);
    for (std::shared_ptr<Cell> cell = As<Cell>(lambda_arg_init); cell; cell = As<Cell>(cell->GetSecond())) {
        if (!Is<Symbol>(cell->GetFirst())) {
            throw RuntimeError("\"BuildLambdaCodegen\" error: first argument (lambda arguments): not a symbol met");
        }
        std::string argument_name = As<Symbol>(cell->GetFirst())->GetName();
        arguments_idx_to_name.push_back(argument_name);
    }

    for (int idx = 0; idx < arguments_idx_to_name.size(); ++idx) {
        llvm::Value* argument_value = new_function->getArg(idx);
        lambda_call_and_self_scope->SetVariableValueCodegen(arguments_idx_to_name[idx], argument_value, true);
    }

    // Handle lambda body
    if (!Is<Cell>(arguments[1])) {
        throw RuntimeError("\"BuildLambdaCodegen\" error: second argument (body) is not a list");
    }

    llvm::Value* return_value = nullptr;
    for (number_t argument_idx = 1; argument_idx < arguments.size(); ++argument_idx) {
        if (argument_idx + 1 == arguments.size() && Interp::CheckIfCellIsLambda(arguments[argument_idx])) {
            std::cout << "last command lambda found" << std::endl;
            std::shared_ptr<Cell> lambda_cell = As<Cell>(arguments[argument_idx]);
            ObjectPtr function = Codegen::BuildLambdaCodegen(std::nullopt, lambda_cell->GetSecond(), lambda_call_and_self_scope);
            return_value = function->Codegen({}, lambda_call_and_self_scope);
        } else {
            return_value = arguments[argument_idx]->Codegen({}, lambda_call_and_self_scope);
        }
    }

    std::vector<llvm::Value*> return_value_scheme_object_indices {
        context.builder->getInt32(0), // because there is no array, so just the object itself
    };
    llvm::Value* return_value_scheme_object_field = context.builder->CreateGEP(context.object_type, return_value, return_value_scheme_object_indices);
    llvm::Value* return_value_scheme_object = context.builder->CreateLoad(context.object_type, return_value_scheme_object_field);
    context.builder->CreateRet(return_value_scheme_object);

    llvm::verifyFunction(*new_function);
    context.builder->SetInsertPoint(old_insert_point);
    return std::make_shared<LambdaCodegen>(new_function);
}

std::pair<std::string, ObjectPtr> BuildLambdaSugarCodegen(std::vector<ObjectPtr> parts, ScopePtr scope) {
    if (parts.size() != 2) {
        throw SyntaxError("Exactly 2 arguments required for \"BuildLambdaSugarCodegen\" function");
    }
    
    auto& context = Codegen::Context::Get();

    std::vector<ObjectPtr> arguments = Interp::ListToVector(parts[0]);
    ObjectPtr command = parts[1];

    ScopePtr lambda_call_and_self_scope = std::make_shared<Scope>();
    lambda_call_and_self_scope->SetPreviousScope(scope);

    std::string lambda_name = As<Symbol>(arguments[0])->GetName();

    // LLVM CTORS
    int temp_argument_counter = 0; // TODO: fix
    for (size_t argument_idx = 1; argument_idx < arguments.size(); ++argument_idx) {
        ++temp_argument_counter;
    }
    int arguments_count = temp_argument_counter;
    std::vector<llvm::Type*> new_function_arguments(arguments_count, context.object_ptr_type);
    llvm::FunctionType* new_function_type = llvm::FunctionType::get(context.object_type, new_function_arguments, false);
    llvm::Function* new_function = llvm::Function::Create(new_function_type, llvm::Function::ExternalLinkage, "LambdaFunction", context.llvm_module.value());
    assert(new_function->empty());

    scope->SetVariableFunction(lambda_name, std::make_shared<LambdaCodegen>(new_function));
    
    // SET INSERT POINT
    llvm::BasicBlock* old_insert_point = context.builder->GetInsertBlock();
    auto& llvm_context = context.builder->getContext();
    llvm::BasicBlock* new_function_main = llvm::BasicBlock::Create(llvm_context, "entry", new_function);
    context.builder->SetInsertPoint(new_function_main);

    // HANDLE ARGUMENTS
    std::vector<std::string> arguments_idx_to_name{};

    for (size_t argument_idx = 1; argument_idx < arguments.size(); ++argument_idx) {
        if (!Is<Symbol>(arguments[argument_idx])) {
            throw SyntaxError("\"BuildLambdaSugarCodegen\" function error: argument passed is not a symbol");
        }
        std::string argument_name = As<Symbol>(arguments[argument_idx])->GetName();
        arguments_idx_to_name.push_back(argument_name);
    }

    for (int idx = 0; idx < arguments_idx_to_name.size(); ++idx) {
        llvm::Value* argument_value = new_function->getArg(idx);
        lambda_call_and_self_scope->SetVariableValueCodegen(arguments_idx_to_name[idx], argument_value, true);
    }

    // Handle lambda body
    llvm::Value* return_value = nullptr;
    return_value = command->Codegen({}, lambda_call_and_self_scope);

    std::vector<llvm::Value*> return_value_scheme_object_indices {
        context.builder->getInt32(0), // because there is no array, so just the object itself
    };
    llvm::Value* return_value_scheme_object_field = context.builder->CreateGEP(context.object_type, return_value, return_value_scheme_object_indices);
    llvm::Value* return_value_scheme_object = context.builder->CreateLoad(context.object_type, return_value_scheme_object_field);
    context.builder->CreateRet(return_value_scheme_object);

    llvm::verifyFunction(*new_function);
    context.builder->SetInsertPoint(old_insert_point);
    return std::make_pair(lambda_name, std::make_shared<LambdaCodegen>(new_function));
}

} // namespace Codegen