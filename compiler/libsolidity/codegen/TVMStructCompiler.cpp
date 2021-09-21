/*
 * Copyright 2018-2019 TON DEV SOLUTIONS LTD.
 *
 * Licensed under the  terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the  GNU General Public License for more details at: https://www.gnu.org/licenses/gpl-3.0.html
 */
/**
 * @author TON Labs <connect@tonlabs.io>
 * @date 2019
 * Struct compiler for TVM
 */

#include "TVMCommons.hpp"
#include "TVMPusher.hpp"
#include "TVMStructCompiler.hpp"
#include "TVMABI.hpp"

using namespace solidity::frontend;

StructCompiler::StructCompiler(StackPusher *pusher, TupleType const* tuple) :
	pusher{pusher}
{
	int i = 0;
	for (auto t :tuple->components()) {
		memberNames.push_back(toString(i++));
		memberTypes.push_back(t);
	}
}

std::vector<Type const*>
getTypesFrom(StructDefinition const *structDefinition)
{
	std::vector<Type const*> result;
	for (const ASTPointer<VariableDeclaration>& vd : structDefinition->members()) {
		result.push_back(vd->type());
	}
	return result;
}

std::vector<std::string>
getNamesFrom(StructDefinition const *structDefinition)
{
	std::vector<std::string> result;
	for (const ASTPointer<VariableDeclaration>& vd : structDefinition->members()) {
		result.push_back(vd->name());
	}
	return result;
}

StructCompiler::StructCompiler(StackPusher *pusher, StructType const *structType) :
	StructCompiler{pusher, getTypesFrom(&structType->structDefinition()), getNamesFrom(&structType->structDefinition())} {
}

StructCompiler::StructCompiler(
	StackPusher *pusher,
	const std::vector<Type const*>& memberTypes,
	const std::vector<std::string>& memberNames
) :
	memberNames{memberNames},
	memberTypes{memberTypes},
	pusher{pusher}
{

}

void StructCompiler::createDefaultStruct(bool resultIsBuilder) {
	for (Type const* type : memberTypes) {
		pusher->pushDefaultValue(type, false);
	}
	pusher->tuple(memberTypes.size());
	if (resultIsBuilder) {
		// TODO generate builder without default value
		tupleToBuilder();
	}
}

void StructCompiler::pushMember(const std::string &memberName) {
	// struct
	pusher->indexNoexcep(getIndex(memberName));
}

void StructCompiler::setMemberForTuple(const std::string &memberName) {
	pusher->setIndex(getIndex(memberName));
}

void StructCompiler::structConstructor(
	std::vector<ASTPointer<ASTString>> const& names,
	const std::function<void(int, Type const*)>& pushParam
) {
	if (names.empty()) {
		int i{};
		for (const Type *type : memberTypes) {
			if (type->category() == Type::Category::Mapping) {
				pusher->pushDefaultValue(type, false);
			} else {
				pushParam(i++, type);
			}
		}
	} else {
		int i = 0;
		for (const Type *type : memberTypes) {
			const std::string& memberName = memberNames.at(i);
			auto it = std::find_if(names.begin(), names.end(), [&](const ASTPointer<ASTString>& name){
				return memberName == *name;
			});
			if (type->category() == Type::Category::Mapping) {
				solAssert(it == names.end(), "");
				pusher->pushDefaultValue(type);
			} else {
				int index = it - names.begin();
				pushParam(index, type);
			}
			++i;
		}
	}

	pusher->tuple(memberTypes.size());
}

void StructCompiler::tupleToBuilder() {
    // stack: tuple
	const int ss = pusher->stackSize();

	pusher->startContinuation();

	const int n = memberNames.size();
	pusher->untuple(n);
	if (n >= 2) {
		pusher->reverse(n, 0);
	}
	pusher->push(+1, "NEWC");

	ChainDataEncoder encoder{pusher};
	EncodePosition position{0, memberTypes};
	encoder.encodeParameters(memberTypes, position);
	pusher->callRef(1, 1);
	// stack: builder
	const int curSize = pusher->stackSize();

	solAssert(ss == curSize, "");
}

void StructCompiler::convertSliceToTuple() {
	const int ss = pusher->stackSize();

	ChainDataDecoder decoder{pusher};
	decoder.decodeData(memberTypes, 0, false);
	pusher->tuple(memberTypes.size());

	solAssert(ss == pusher->stackSize(), "");
}

int StructCompiler::getIndex(const std::string& name) {
	int index = std::find(memberNames.begin(), memberNames.end(), name) - memberNames.begin();
	solAssert(index != static_cast<int>(memberNames.size()), "");
	return index;
}