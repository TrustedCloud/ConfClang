#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"


#include "clang/Driver/Options.h" 
#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <sstream>
#include <fstream>


using namespace clang::tooling;
using namespace clang::driver;
using namespace clang;
using namespace llvm;



std::stringstream os;
std::string public_char = "public ";
std::string private_char = "private ";

class SignatureDumpVisitor : public RecursiveASTVisitor<SignatureDumpVisitor> {
private:
	ASTContext *Context;
public:
	bool isSgxPrivate(QualType ty) {
		if (const AttributedType *AT = dyn_cast<AttributedType>(ty.getTypePtr())) {
			if (AT->getAttrKind() == AttributedType::attr_sgx_private) {
				return true;
			}
			else {
				isSgxPrivate(AT->desugar());
			}
		}
		else if (const TypedefType *TD = dyn_cast<TypedefType>(ty.getTypePtr())) {
			return isSgxPrivate(TD->desugar());
		}
		return false;
	}
	std::string dumpType(QualType ty) {
		std::stringstream ss;
		while (ty->isPointerType()) {
			if (isSgxPrivate(ty))
				ss << private_char;
			else
				ss << public_char;
			ty = ty->getPointeeType();
		}	
		if (isSgxPrivate(ty))
			ss << private_char;
		else
			ss << public_char;
		return ss.str();
	}

	bool VisitFunctionDecl(FunctionDecl *FD) {
		if (!FD->isThisDeclarationADefinition())
			return true;
		os << FD->getName().str() << std::endl;
		QualType RT = FD->getReturnType();
		os << "\t" << dumpType(RT) << std::endl;
		for (FunctionDecl::param_iterator p = FD->param_begin(); p != FD->param_end(); p++) {
			QualType pt = (*p)->getType();
			os << "\t" << dumpType(pt) << std::endl;
		}
		return true;
	}
	explicit SignatureDumpVisitor(ASTContext *Context) : Context(Context) {}

};


class SignatureDumpConsumer : public clang::ASTConsumer {
private:
	SignatureDumpVisitor Visitor;
public:
	virtual void HandleTranslationUnit(clang::ASTContext &Context) {
		Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	}
	explicit SignatureDumpConsumer(ASTContext *Context) : Visitor(Context) {}
};


class SignatureDumpAction : public clang::ASTFrontendAction {
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
		return std::unique_ptr<clang::ASTConsumer>(
			new SignatureDumpConsumer(&Compiler.getASTContext()));
	}
};

static llvm::cl::OptionCategory SignatureDumpCategory("signature-dump options");

int main(int argc, const char **argv) {
	if (argc > 1) {
		CommonOptionsParser OptionsParser(argc, argv, SignatureDumpCategory);
		ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
		Tool.run(newFrontendActionFactory<SignatureDumpAction>().get());
		std::fstream f;
		f.open("signatures.md", std::ios::out);
		f << os.str();
		f.close();
		return 0;
	}
}
