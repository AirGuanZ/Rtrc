#include <Rtrc/Core/Serialization/TextSerializer.h>
#include <Rtrc/Graphics/Shader/ShaderDatabase.h>

RTRC_BEGIN

ShaderDatabase::ShaderDatabase()
{
    RegisterAllPreprocessedShadersInShaderDatabase(*this);
}

RC<Shader> ShaderDatabase::ShaderTemplate::GetVariant(const FastKeywordContext &keywords, bool persistent)
{
    const FastKeywordSetValue keywordValue = record_->keywordSet.ExtractValue(keywords);
    return GetVariant(keywordValue, persistent);
}

RC<Shader> ShaderTemplate::GetVariant(bool persistent)
{
    return GetVariant(FastKeywordSetValue{}, persistent);
}

RC<Shader> ShaderDatabase::ShaderTemplate::GetVariant(const FastKeywordSetValue &keywords, bool persistent)
{
    if(persistent)
    {
        const uint32_t persistentIndex = keywords.GetInternalValue();
        {
            std::shared_lock lock(record_->shaderTemplateMutex);
            if(persistentShaders[persistentIndex])
            {
                return persistentShaders[persistentIndex];
            }
        }
        std::lock_guard lock(record_->shaderTemplateMutex);
        if(persistentShaders[persistentIndex])
        {
            return persistentShaders[persistentIndex];
        }
        persistentShaders[persistentIndex] = database_->GetShaderImpl(record_, keywords);
        return persistentShaders[persistentIndex];
    }
    return database_->GetShaderImpl(record_, keywords);
}

const FastKeywordSet &ShaderDatabase::ShaderTemplate::GetKeywordSet() const
{
    return record_->keywordSet;
}

bool ShaderDatabase::ShaderTemplate::HasBuiltinKeyword(BuiltinKeyword keyword) const
{
    return record_->hasBuiltinKeyword[std::to_underlying(keyword)];
}

void ShaderDatabase::SetDevice(Ref<Device> device)
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
    for(size_t i = 0; i < shader.keywords.size(); ++i)
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

    record->name         = pooledName;
    record->keywordSet   = FastKeywordSet(keywordRecords);
    record->parsedShader = std::move(shader);
    
    for(int i = 0; i < EnumCount<BuiltinKeyword>; ++i)
    {
        const auto keyword = GetBuiltinShaderKeyword(static_cast<BuiltinKeyword>(i));
        record->hasBuiltinKeyword[i] = record->keywordSet.HasKeyword(keyword);
    }
}

RC<ShaderDatabase::ShaderTemplate> ShaderDatabase::GetShaderTemplate(GeneralPooledString name, bool persistent)
{
    auto it = records_.find(name);
    if(it == records_.end())
    {
        return nullptr;
    }
    auto record = it->second.get();

    auto CreateShaderTemplate = [&]
    {
        auto ret = MakeRC<ShaderTemplate>();
        ret->database_ = this;
        ret->record_ = record;
        ret->persistentShaders.resize(1 << record->keywordSet.GetTotalBitCount());
        return ret;
    };

    if(persistent)
    {
        {
            std::shared_lock lock(record->shaderTemplateMutex);
            if(record->shaderTemplate)
            {
                return record->shaderTemplate;
            }
        }
        std::lock_guard lock(record->shaderTemplateMutex);
        if(record->shaderTemplate)
        {
            return record->shaderTemplate;
        }
        record->shaderTemplate = CreateShaderTemplate();
        return record->shaderTemplate;
    }

    return CreateShaderTemplate();
}

RC<ShaderDatabase::ShaderTemplate> ShaderDatabase::GetShaderTemplate(std::string_view name, bool persistent)
{
    return GetShaderTemplate(GeneralPooledString(name), persistent);
}

ShaderDatabase::ShaderKey::ShaderKey(GeneralPooledString name, FastKeywordSetValue keywords)
    : keyValue((static_cast<uint64_t>(name.GetIndex()) << 32) | keywords.GetInternalValue())
{
    
}

RC<Shader> ShaderDatabase::GetShaderImpl(ShaderRecord *record, FastKeywordSetValue fastKeywordValues)
{
    auto GetOrCreateImpl = [&]
    {
        const ShaderKey key(record->name, fastKeywordValues);
        return shaders_.GetOrCreate(key, [&]
        {
            const ParsedShader &parsedShader = record->parsedShader;

            assert(static_cast<int>(parsedShader.keywords.size()) == record->keywordSet.GetKeywordCount());
            std::vector<ShaderKeywordValue> keywordValues(parsedShader.keywords.size());
            for(size_t i = 0; i < parsedShader.keywords.size(); ++i)
            {
                const int value = record->keywordSet.ExtractSingleKeywordValue(static_cast<int>(i), fastKeywordValues);
                keywordValues[i].value = value;
            }
            const int variantIndex = ComputeVariantIndex(parsedShader.keywords, keywordValues);
            const ParsedShaderVariant variant = parsedShader.variants[variantIndex];

            CompilableShader compilableShader;
            compilableShader.envir                   = envir_;
            compilableShader.name                    = parsedShader.name;
            compilableShader.source                  = parsedShader.GetCachedSource();
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
            
            return compiler_.Compile(compilableShader, debug_);
        });
    };

    return GetOrCreateImpl();
}

RTRC_END
