#include <filesystem>
#include <iostream>
#include <queue>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4624)
#pragma warning(disable: 4291)
#pragma warning(disable: 4146)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#endif
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <Reflection/Frontend.h>

namespace FrontendDetail
{

    using namespace clang;
    using namespace ast_matchers;
    using namespace llvm;

    const Type *RemoveTypedefs(const Type *type)
    {
        while(true)
        {
            if(auto tt = type->getAs<TypedefType>())
            {
                type = tt->desugar().getTypePtr();
                continue;
            }
            if(auto tt = type->getAs<UsingType>())
            {
                type = tt->desugar().getTypePtr();
                continue;
            }
            break;
        }
        return type;
    }

    class MatchCallback : public MatchFinder::MatchCallback
    {

    public:

        struct ResultPerTranslateUnit
        {
            std::vector<Struct> structs;
        };

        void run(const MatchFinder::MatchResult &matchResult) override
        {
            auto record = matchResult.Nodes.getNodeAs<CXXRecordDecl>("id");
            if(!record)
                return;
            
            if(!record->hasDefinition() || record->isTemplated())
                return;

            if(record->isTemplated())
                return;
            
            std::set<std::string> annos;
            for(auto attr : record->attrs())
            {
                auto anno = dyn_cast_or_null<AnnotateAttr>(attr);
                if(anno && anno->getAnnotationLength() > 0)
                    annos.insert(anno->getAnnotation().str());
            }

            std::string qualifiedName = record->getQualifiedNameAsString();
            Struct *pOutRecord = nullptr;
            for(auto &r : results_.back().structs)
            {
                if(r.qualifiedName == qualifiedName)
                {
                    pOutRecord = &r;
                    break;
                }
            }
            if(!pOutRecord)
            {
                pOutRecord = &results_.back().structs.emplace_back();
            }
            else if(!pOutRecord->fields.empty())
            {
                return;
            }
            
            assert(!results_.empty());
            auto &outRecord = *pOutRecord;
            outRecord.qualifiedName = std::move(qualifiedName);
            outRecord.annotations = std::move(annos);
            outRecord.fields.clear();
            for(auto field : record->fields())
            {
                auto &outField = outRecord.fields.emplace_back();
                auto type = field->getType().getTypePtr();
                assert(type);
                type = RemoveTypedefs(type);

                if(type->getTypeClass() == Type::ConstantArray)
                {
                    auto cat = cast<ConstantArrayType>(type);
                    outField.arraySize = static_cast<int>(cat->getSize().getSExtValue());
                    type = cat->getElementType().getTypePtr();
                    type = RemoveTypedefs(type);
                }

                std::string templateArgStr;
                if(auto tt = type->getAs<TemplateSpecializationType>())
                {
                    templateArgStr = "<";
                    bool isFirst = true;
                    for(auto arg : tt->template_arguments())
                    {
                        if(isFirst)
                            isFirst = false;
                        else
                            templateArgStr += ", ";

                        // TODO: Support other kinds of template arguments
                        if(arg.getKind() == TemplateArgument::ArgKind::Type)
                        {
                            auto ntt = RemoveTypedefs(arg.getAsType().getTypePtr());
                            templateArgStr += QualType(ntt, 0).getAsString();
                        }
                    }
                    templateArgStr += ">";
                }

                if(auto rt = type->getAsCXXRecordDecl())
                {
                    outField.typeStr = rt->getQualifiedNameAsString();
                }
                else
                {
                    outField.typeStr = field->getType().getAsString();
                }
                outField.typeStr += templateArgStr;

                if(outField.typeStr == "uint")
                    outField.typeStr = "unsigned int";
                
                static const std::map<std::string, FieldKind> STR_TO_TYPE =
                {
                    { "float",                       FieldKind::Float    },
                    { "Rtrc::Vector2<float>",        FieldKind::Float2   },
                    { "Rtrc::Vector3<float>",        FieldKind::Float3   },
                    { "Rtrc::Vector4<float>",        FieldKind::Float4   },
                    { "int",                         FieldKind::Int      },
                    { "Rtrc::Vector2<int>",          FieldKind::Int2     },
                    { "Rtrc::Vector3<int>",          FieldKind::Int3     },
                    { "Rtrc::Vector4<int>",          FieldKind::Int4     },
                    { "unsigned int",                FieldKind::UInt     },
                    { "Rtrc::Vector2<unsigned int>", FieldKind::UInt2    },
                    { "Rtrc::Vector3<unsigned int>", FieldKind::UInt3    },
                    { "Rtrc::Vector4<unsigned int>", FieldKind::UInt4    },
                    { "Rtrc::Matrix4x4f",            FieldKind::Float4x4 },
                };
                if(auto it = STR_TO_TYPE.find(outField.typeStr); it != STR_TO_TYPE.end())
                    outField.kind = it->second;
                else
                    outField.kind = FieldKind::Others;

                outField.name = field->getNameAsString();
            }
            
            const SourceLocation loc = record->getLocation();
            const PresumedLoc presumedLoc = matchResult.SourceManager->getPresumedLoc(loc);
            if(presumedLoc.isValid())
            {
                std::string filename = presumedLoc.getFilename();
                if(!filename.empty())
                {
                    auto path = std::filesystem::path(filename);
                    outRecord.sourceFilename = absolute(path).lexically_normal().string();
                }
            }
        }

        void onStartOfTranslationUnit() override
        {
            results_.emplace_back();
        }
        
        std::span<const ResultPerTranslateUnit> GetResults() const
        {
            return std::span(results_);
        }

    private:

        std::vector<ResultPerTranslateUnit> results_;
    };

    class FrontendAction : public ASTFrontendAction
    {
    public:
        
        explicit FrontendAction(MatchCallback *callback)
        {
            const DeclarationMatcher recordMatcher = cxxRecordDecl(decl().bind("id"), hasAttr(attr::Annotate));
            finder_.addMatcher(recordMatcher, callback);
        }

        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override
        {
            return finder_.newASTConsumer();
        }

    private:
        
        MatchFinder finder_;
    };

    class FrontendActionFactory : public tooling::FrontendActionFactory
    {
    public:

        explicit FrontendActionFactory(MatchCallback *callback)
            : callback_(callback)
        {

        }

        std::unique_ptr<clang::FrontendAction> create() override
        {
            return std::make_unique<FrontendAction>(callback_);
        }

    private:

        MatchCallback *callback_;
    };

    struct StructRecord
    {
        int index = -1;
        Struct rawStruct;
        std::set<const StructRecord *, std::less<>> prevs;
        std::set<const StructRecord *, std::less<>> succs;
    };

    struct StructDatabase
    {
        std::vector<std::unique_ptr<StructRecord>> records;
        std::map<std::string, int> nameToRecord;

        const StructRecord *Register(const Struct &s)
        {
            if(const auto it = nameToRecord.find(s.qualifiedName); it != nameToRecord.end())
            {
                if(s != records[it->second]->rawStruct)
                    throw std::runtime_error("Found different definitions of " + s.qualifiedName);
                return records[it->second].get();
            }
            const int index = static_cast<int>(records.size());
            auto &record = records.emplace_back();
            record = std::make_unique<StructRecord>();
            record->index = index;
            record->rawStruct = s;
            nameToRecord.insert({ s.qualifiedName, index });
            return record.get();
        }

        void AddArc(const StructRecord *a, const StructRecord *b)
        {
            assert(a->succs.contains(b) == b->prevs.contains(a));
            assert(a->prevs.contains(b) == b->succs.contains(a));
            records[a->index]->succs.insert(b);
            records[b->index]->prevs.insert(a);
        }

        std::vector<const StructRecord *> GetSortedRecords() const
        {
            std::vector<std::set<const StructRecord *>> realSuccs(records.size());
            for(auto &record : records)
            {
                for(auto succ : record->succs)
                {
                    if(!succ->succs.contains(record.get()))
                        realSuccs[record->index].insert(succ);
                }
            }

            std::vector<int> unresolvedPrevCount(records.size(), 0);
            for(auto &succs : realSuccs)
            {
                for(auto succ : succs)
                    ++unresolvedPrevCount[succ->index];
            }

            std::queue<int> readyRecords;
            for(int i = 0; i < static_cast<int>(records.size()); ++i)
            {
                if(!unresolvedPrevCount[i])
                    readyRecords.push(i);
            }

            std::vector<const StructRecord *> ret;
            while(!readyRecords.empty())
            {
                const int index = readyRecords.front();
                readyRecords.pop();
                ret.push_back(records[index].get());
                for(auto succ : realSuccs[index])
                {
                    if(!--unresolvedPrevCount[succ->index])
                        readyRecords.push(succ->index);
                    assert(unresolvedPrevCount[succ->index] >= 0);
                }
            }

            if(ret.size() != records.size())
                throw std::runtime_error("Cycle detected between struct definitions");
            return ret;
        }
    };

} // namespace FrontendDetail

std::vector<Struct> ParseStructs(const SourceInfo &sourceInfo)
{
    std::vector<std::string> commandLine;
    commandLine.push_back("-ferror-limit=0");
    commandLine.push_back("-DRTRC_REFLECTION_TOOL=1");
    commandLine.push_back("-std=c++23");
    for(auto &dir : sourceInfo.includeDirs)
    {
        commandLine.push_back("-I");
        commandLine.push_back(dir);
    }
    clang::tooling::FixedCompilationDatabase compilationDatabase(".", commandLine);
    clang::tooling::ClangTool tool(compilationDatabase, sourceInfo.filenames);

    auto ignoreDiagnostic = std::make_unique<clang::IgnoringDiagConsumer>();
    tool.setDiagnosticConsumer(ignoreDiagnostic.get());

    FrontendDetail::MatchCallback callback;
    tool.run(std::make_unique<FrontendDetail::FrontendActionFactory>(&callback).get());

    FrontendDetail::StructDatabase database;
    for(auto &translationUnit : callback.GetResults())
    {
        const FrontendDetail::StructRecord *prev = nullptr;
        for(auto &s : translationUnit.structs)
        {
            const auto curr = database.Register(s);
            if(prev)
                database.AddArc(prev, curr);
            prev = curr;
        }
    }

    auto sortedRecords = database.GetSortedRecords();

    std::vector<Struct> ret;
    for(auto record : database.GetSortedRecords())
        ret.push_back(record->rawStruct);

    return ret;
}
