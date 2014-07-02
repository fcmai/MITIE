// Copyright (C) 2014 Massachusetts Institute of Technology, Lincoln Laboratory
// License: Boost Software License   See LICENSE.txt for the full license.
// Authors: Davis E. King (davis.king@ll.mit.edu)
#ifndef MIT_LL_MITIE_NER_TRAINER_H__
#define MIT_LL_MITIE_NER_TRAINER_H__

#include <vector>
#include <string>
#include <utility>
#include <mitie/total_word_feature_extractor.h>
#include <mitie/named_entity_extractor.h>
#include <dlib/svm.h>
#include <map>

namespace mitie
{

// ----------------------------------------------------------------------------------------

    class ner_training_instance
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This object represents an annotated list of string tokens.  The annotations
                indicate where named entities appear in the text.  Therefore, this object
                is used to create training data that is then given to the ner_trainer
                object defined below.
        !*/

    public:
        explicit ner_training_instance (
            const std::vector<std::string>& tokens
        );
        /*!
            ensures
                - #num_tokens() == tokens.size()
                - This object will represent a chunk of text made up of the given tokens.
                  You can add entity mentions to it by calling any of the add_entity()
                  methods.
        !*/

        unsigned long num_tokens(
        ) const;
        /*!
            ensures
                - returns the number of text tokens in this training instance.
        !*/

        unsigned long num_entities(
        ) const;
        /*!
            ensures
                - returns the number of named entities that have been added into this
                  training instance.
        !*/

        bool overlaps_any_entity (
            unsigned long start,
            unsigned long length
        ) const;
        /*!
            requires
                - length > 0
                - start+length-1 < num_tokens()
            ensures
                - This function checks if any of the entity annotations in the given
                  instance overlap with the entity starting at token start and consisting
                  of length number of tokens.
                - returns true if any entities overlap and false otherwise.
        !*/

        void add_entity (
            const std::pair<unsigned long,unsigned long>& range,
            const std::string& label
        );
        /*!
            requires
                - overlaps_any_entity(range.first, range.second-range.first) == false
                - range.first < range.second <= num_tokens()
            ensures
                - Adds the given entity into this object as a NER training entity.
                - Interprets range as a half-open range rather than as a start/length pair.
                - #num_entities() == num_entities() + 1
        !*/

        void add_entity (
            unsigned long start,
            unsigned long length,
            const char* label
        );
        /*!
            requires
                - overlaps_any_entity(start, length) == false
                - length > 0
                - start+length <= num_tokens()
            ensures
                - Adds the given entity into this object as a NER training entity.  The
                  entity begins at token with index start and consists of length tokens.
                - #num_entities() == num_entities() + 1
        !*/

    private:
        friend class ner_trainer;
        const std::vector<std::string> tokens;
        std::vector<std::pair<unsigned long, unsigned long> > chunks;
        std::vector<std::string> chunk_labels;
    };

// ----------------------------------------------------------------------------------------

    class ner_trainer
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a tool for training mitie::named_entity_extractor objects from a 
                set of annotated training data.
        !*/
    public:
        explicit ner_trainer (
            const std::string& filename
        );
        /*!
            ensures
                - #get_beta() == 0.5
                - #num_threads() == 16
                - This function attempts to load a mitie::total_word_feature_extractor from the
                  file with the given filename.  This feature extractor is used during the
                  NER training process.  
        !*/

        unsigned long size(
        ) const;
        /*!
            ensures
                - returns the number of training instances that have been added into this object.
        !*/

        void add (
            const ner_training_instance& item
        );
        /*!
            ensures
                - #size() == size() + 1
                - Adds the given training instance into this object.  It will be used to create
                  a named_entity_extractor when train() is called.
        !*/

        void add (
            const std::vector<std::string>& tokens,
            const std::vector<std::pair<unsigned long,unsigned long> >& ranges,
            const std::vector<std::string>& labels
        );
        /*!
            requires
                - ranges.size() == labels.size()
                - None of the elements of ranges can overlap each other. 
                - for all valid i:
                    - ranges[i].first < ranges[i].second <= tokens.size() 
            ensures
                - This function is a convenient way to add a bunch of
                  ner_training_instances all at once.  In particular, it is equivalent to
                  creating a ner_training_instance(tokens), adding all the
                  ranges[i],labels[i] pairs to it and then calling add().
        !*/

        void add (
            const std::vector<std::vector<std::string> >& tokens,
            const std::vector<std::vector<std::pair<unsigned long,unsigned long> > >& ranges,
            const std::vector<std::vector<std::string> >& labels
        );
        /*!
            requires
                - it must be legal to call add(tokens[i], ranges[i], labels[i]) for all i.
            ensures
                - For all valid i: performs:  add(tokens[i], ranges[i], labels[i]);
                  (i.e. This function is just a convenience for adding a bunch of training
                  data into a trainer in one call).
        !*/

        unsigned long get_num_threads (
        ) const;
        /*!
            ensures
                - returns the number of threads that will be used to perform training.  You 
                  should set this equal to the number of processing cores you have on your 
                  computer.  
        !*/

        void set_num_threads (
            unsigned long num
        );
        /*!
            ensures
                - #get_num_threads() == num
        !*/

        double get_beta (
        ) const;
        /*!
            ensures
                - returns the trainer's beta parameter.  This parameter controls the
                  trade-off between trying to avoid false alarms but also detecting
                  everything.  Different values of beta have the following interpretations:
                    - beta < 1 indicates that you care more about avoiding false alarms
                      than missing detections.  The smaller you make beta the more the
                      trainer will try to avoid false alarms.
                    - beta == 1 indicates that you don't have a preference between avoiding
                      false alarms or not missing detections.  That is, you care about
                      these two things equally.
                    - beta > 1 indicates that care more about not missing detections than
                      avoiding false alarms.
        !*/

        void set_beta (
            double new_beta
        );
        /*!
            requires
                - new_beta >= 0
            ensures
                - #get_beta() == new_beta
        !*/

        named_entity_extractor train (
        ) const;
        /*!
            requires
                - size() > 0
            ensures
                - Trains a named_entity_extractor based on the training instances given to
                  this object via add() calls and returns the result.
        !*/

    private:

        unsigned long count_of_least_common_label (
            const std::vector<unsigned long>& labels
        ) const;

        typedef dlib::multiclass_linear_decision_function<dlib::sparse_linear_kernel<ner_sample_type>,unsigned long> classifier_type;
        classifier_type train_ner_segment_classifier (
            const std::vector<ner_sample_type>& samples,
            const std::vector<unsigned long>& labels
        ) const;

        void extract_ner_segment_feats (
            const dlib::sequence_segmenter<ner_feature_extractor>& segmenter,
            std::vector<ner_sample_type>& samples,
            std::vector<unsigned long>& labels
        ) const;

        void train_segmenter (
            dlib::sequence_segmenter<ner_feature_extractor>& segmenter
        ) const;

        unsigned long get_label_id (
            const std::string& str
        );

        std::vector<std::string> get_all_labels(
        ) const;


        total_word_feature_extractor tfe;
        double beta;
        unsigned long num_threads;
        std::map<std::string,unsigned long> label_to_id;
        std::vector<std::vector<std::string> > sentences;
        std::vector<std::vector<std::pair<unsigned long, unsigned long> > > chunks;
        std::vector<std::vector<unsigned long> > chunk_labels;
    };

// ----------------------------------------------------------------------------------------

}

#endif // MIT_LL_MITIE_NER_TRAINER_H__
