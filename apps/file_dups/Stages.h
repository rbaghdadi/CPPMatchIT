//
// Created by Jessica Ray on 2/2/16.
//

#ifndef MATCHIT_STAGES_H
#define MATCHIT_STAGES_H

/*
 * User-defined datatypes
 */
typedef struct {
    char *file_path;
    unsigned char *md5_hash;
} FileHash;

typedef struct {
    bool is_match;
    char *file_path1;
    char *file_path2;
} Comparison;


/*
 * Stage class definitions
 */

// Transforms

class IdentityTransform : public TransformStage<char *, char *> {
public:
    IdentityTransform(char * (*transform)(char *), JIT *jit) :
            TransformStage(transform, "identity", jit) {}
};

class Transform : public TransformStage<char *, FileHash *> {
public:
    // TODO this isn't very nice for the user to have to write
    Transform(FileHash *(*transform)(char *), JIT *jit, std::vector<MType *> filehash_fields) :
            TransformStage(transform, "transform", jit, std::vector<MType *>(), filehash_fields) {}
};

class StructCheck : public TransformStage<FileHash *, FileHash *> {
public:
    StructCheck(FileHash *(*transform)(FileHash *), JIT *jit, std::vector<MType *> filehash_fields) :
            TransformStage(transform, "struct_check", jit, filehash_fields, filehash_fields) {}
};

// Filters

class Filter : public FilterStage<char *> {
public:
    Filter(bool (*filter)(char*), std::string transform_name, JIT *jit) : FilterStage(filter, transform_name, jit) {}
};

// Comparators

class HashCompare : public ComparisonStage<FileHash *, Comparison *> {
public:
    HashCompare(Comparison *(*compare)(FileHash *, FileHash *), JIT *jit, std::vector<MType *> filehash_fields,
                std::vector<MType *> comparison_fields) : ComparisonStage(compare, "compare", jit, filehash_fields, comparison_fields) {}
};


/*
 * Stage computations
 */

extern "C" char *identity(char *file_path) {
    std::cerr << "Running function identity on " << file_path << std::endl;
    return file_path;
}

extern "C" FileHash *struct_check(FileHash *in) {
    std::cerr << "In struct_check. File path is " << in->file_path << std::endl;
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int) in->md5_hash[i]);
    }
    fprintf(stderr, "md5 digest: %s\n", md5_str);
    return in;
}

// TODO need to free this stuff
extern "C" FileHash* transform(char *file_path) {
    std::cerr << "Transform " << file_path;
    FILE *file = fopen(file_path, "rt");
    MD5_CTX md5_context;
    int bytes_read;
    unsigned char buffer[1024];

    FileHash *md5_hash = (FileHash *) malloc(sizeof(FileHash));
    md5_hash->file_path = (char *) malloc(sizeof(char) * 150);
    md5_hash->md5_hash = (unsigned char *) malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH);
    // http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c
    MD5_Init(&md5_context);
    while ((bytes_read = fread(buffer, 1, 1024, file)) != 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }
    MD5_Final(md5_hash->md5_hash, &md5_context);
    strcpy(md5_hash->file_path, file_path);
    fclose(file);
    // http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/
    char md5_str[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int) md5_hash->md5_hash[i]);
    }
    fprintf(stderr, " -> md5 digest: %s\n", md5_str);
    return md5_hash;
}

extern "C" bool llvm_filter(char *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running jpg_filter: " << file_path << " ";
    char *cmp = strstr(file_path, ".ll");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" bool txt_filter(char *file_path) {
    // 0 => not in string, so we can keep it
    std::cerr << "Running txt_filter: " << file_path << " ";
    char *cmp = strstr(file_path, ".txt");
    if (cmp == NULL) {
        std::cerr << "Keep me" << std::endl;
        return true;
    } else {
        std::cerr << "Drop me" << std::endl;
        return false;
    }
}

extern "C" Comparison *compare(FileHash *file1, FileHash *file2) {
    std::cerr << "Comparing: " << file1->file_path << " and " << file2->file_path;
    bool is_match = true;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (file1->md5_hash[i] != file2->md5_hash[i]) {
            is_match = false;
            break;
        }
    }
    Comparison *comparison = (Comparison *) malloc(sizeof(Comparison));
    comparison->file_path1 = (char *) malloc(sizeof(char) * 50);
    comparison->file_path2 = (char *) malloc(sizeof(char) * 50);
    comparison->is_match = is_match;
    strcpy(comparison->file_path1, file1->file_path);
    strcpy(comparison->file_path2, file2->file_path);
    std::cerr << " is match?: " << is_match << std::endl;
    return comparison;
}

#endif //MATCHIT_STAGES_H
