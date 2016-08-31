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

using namespace clang::tooling;
using namespace clang::driver;
using namespace clang;
using namespace llvm;

class DummyPassVisitor : public RecursiveASTVisitor<DummyPassVisitor> {
private:
	ASTContext *Context;
public:

	bool VisitDecl(Decl *D) {
		if (VarDecl *VD = dyn_cast<VarDecl>(D)) {
			QualType T = VD->getType();
			while (1) {
				T.dump();
				if (auto *AT = dyn_cast<AttributedType>(T.getTypePtr())) {
					errs() <<"ATT_KIND = " << AT->getAttrKind() << "\n";
					
				}
				

				if (T->isPointerType()) {
					T = T->getPointeeType();
				}
				else
					break;
			}
		}
		return true;
	}
	explicit DummyPassVisitor(ASTContext *Context) : Context(Context) {}

};


class DummyPassConsumer : public clang::ASTConsumer {
private:
	DummyPassVisitor Visitor;
public:
	virtual void HandleTranslationUnit(clang::ASTContext &Context) {
		Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	}
	explicit DummyPassConsumer(ASTContext *Context) : Visitor(Context) {}
};


class DummyPassAction : public clang::ASTFrontendAction {
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
		return std::unique_ptr<clang::ASTConsumer>(
			new DummyPassConsumer(&Compiler.getASTContext()));
	}
};

static llvm::cl::OptionCategory DummyPassCategory("dummy-pass options");

int main(int argc, const char **argv) {
	if (argc > 1) {
		CommonOptionsParser OptionsParser(argc, argv, DummyPassCategory);
		ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
		return Tool.run(newFrontendActionFactory<DummyPassAction>().get());
	}
}
