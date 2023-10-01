#include <ShaderCompiler/Database/ShaderDatabase.h>

RTRC_SHADER_COMPILER_BEGIN

RC<Shader> ShaderDatabase::ShaderTemplate::GetDefaultVariant()
{
    return GetVariant(FastKeywordSetValue{});
}

RC<Shader> ShaderDatabase::ShaderTemplate::GetVariant(const FastKeywordContext &keywords)
{
    const FastKeywordSetValue keywordValue = record_->keywordSet.ExtractValue(keywords);
    return database_->GetShaderImpl(record_, keywordValue);
}

RC<Shader> ShaderDatabase::ShaderTemplate::GetVariant(const FastKeywordSetValue &keywords)
{
    return database_->GetShaderImpl(record_, keywords);
}

const FastKeywordSet &ShaderDatabase::ShaderTemplate::GetKeywordSet() const
{
    return record_->keywordSet;
}

void ShaderDatabase::SetDevice(ObserverPtr<Device> device)
{
    compiler_.SetDevice(device);
}

void ShaderDatabase::SetDebug(bool debug)
{
    debug_ = debug;
}

void ShaderDatabase::AddIncludeDirectory(std::string dir)
{
    envir_.includeDirs.push_back(std::move(dir));
}

void ShaderDatabase::AddShader(ParsedShader shader)
{
    std::vector<FastKeywordSet::InputRecord> keywordRecords(shader.keywords.size());
    for(size_t i=  0; i < shader.keywords.size(); ++i)
    {
        auto &in = shader.keywords[i];
        auto &out = keywordRecords[i];
        in.Match(
        [&](const BoolShaderKeyword &keyword)
        {
            out.keyword = FastShaderKeyword(keyword.name);
            out.bitCount = 1;
        },
        [&](const EnumShaderKeyword &keyword)
        {
            out.keyword = FastShaderKeyword(keyword.name);
            out.bitCount = 1;
            while((1 << out.bitCount) < keyword.values.size())
            {
                ++out.bitCount;
            }
        });
    }

    const GeneralPooledString pooledName(shader.name);
    auto &record = records_[pooledName];
    record = MakeBox<ShaderRecord>();

    record->name                      = pooledName;
    record->keywordSet                = FastKeywordSet(keywordRecords);
    record->shaderTemplate            = MakeBox<ShaderTemplate>();
    record->shaderTemplate->database_ = this;
    record->shaderTemplate->record_   = record.get();
    record->parsedShader              = std::move(shader);
}

const ShaderDatabase::ShaderTemplate *ShaderDatabase::GetShaderTemplate(GeneralPooledString name)
{
    auto it = records_.find(name);
    return it != records_.end() ? it->second->shaderTemplate.get() : nullptr;
}

const ShaderDatabase::ShaderTemplate *ShaderDatabase::GetShaderTemplate(std::string_view name)
{
    return GetShaderTemplate(GeneralPooledString(name));
}

RC<Shader> ShaderDatabase::GetShader(GeneralPooledString name, const FastKeywordContext &keywords)
{
    auto it = records_.find(name);
    if(it == records_.end())
    {
        return nullptr;
    }
    const FastKeywordSetValue keywordValue = it->second->keywordSet.ExtractValue(keywords);
    return GetShaderImpl(it->second.get(), keywordValue);
}

RC<Shader> ShaderDatabase::GetShader(GeneralPooledString name, const FastKeywordSetValue &keywords)
{
    auto it = records_.find(name);
    if(it == records_.end())
    {
        return nullptr;
    }
    return GetShaderImpl(it->second.get(), keywords);
}

ShaderDatabase::ShaderKey::ShaderKey(GeneralPooledString name, FastKeywordSetValue keywords)
    : keyValue((static_cast<uint64_t>(name.GetIndex()) << 32) | keywords.GetInternalValue())
{
    
}

RC<Shader> ShaderDatabase::GetShaderImpl(const ShaderRecord *record, FastKeywordSetValue fastKeywordValues)
{
    const ShaderKey key(record->name, fastKeywordValues);
    return shaders_.GetOrCreate(key, [&]
    {
        const ParsedShader &parsedShader = record->parsedShader;

        assert(parsedShader.keywords.size() == record->keywordSet.GetKeywordCount());
        std::vector<ShaderKeywordValue> keywordValues(parsedShader.keywords.size());
        for(size_t i = 0; i < parsedShader.keywords.size(); ++i)
        {
            keywordValues[i].value = record->keywordSet.ExtractSingleKeywordValue(i, fastKeywordValues);
        }
        const int variantIndex = ComputeVariantIndex(parsedShader.keywords, keywordValues);
        const ParsedShaderVariant variant = parsedShader.variants[variantIndex];

        CompilableShader compilableShader;
        compilableShader.name                    = parsedShader.name;
        compilableShader.source                  = variant.source;
        compilableShader.sourceFilename          = parsedShader.sourceFilename;
        compilableShader.keywords                = parsedShader.keywords;
        compilableShader.keywordValues           = keywordValues;
        compilableShader.vertexEntry             = variant.vertexEntry;
        compilableShader.fragmentEntry           = variant.fragmentEntry;
        compilableShader.computeEntry            = variant.computeEntry;
        compilableShader.isRayTracingShader      = variant.vertexEntry.empty() &&
                                                   variant.fragmentEntry.empty() &&
                                                   variant.computeEntry.empty();
        compilableShader.entryGroups             = variant.entryGroups;
        compilableShader.bindingGroups           = variant.bindingGroups;
        compilableShader.ungroupedBindings       = variant.ungroupedBindings;
        compilableShader.aliases                 = variant.aliases;
        compilableShader.inlineSamplerDescs      = variant.inlineSamplerDescs;
        compilableShader.inlineSamplerNameToDesc = variant.inlineSamplerNameToDesc;
        compilableShader.pushConstantRanges      = variant.pushConstantRanges;
        
        return compiler_.Compile(envir_, compilableShader, debug_);
    });
}

RTRC_SHADER_COMPILER_END
