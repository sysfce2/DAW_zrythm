;;; SPDX-FileCopyrightText: © 2022-2024 Alexandros Theodotou <alex@zrythm.org>
;;; SPDX-License-Identifier: LicenseRef-ZrythmLicense
;;;
;;; Generate appdata.xml
;;;
;;; Usage: gen-appdata-xml [OUTPUT-FILE]

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (gen-gtk-resources-xml)
  #:use-module (guile-utils)
  #:use-module (ice-9 format)
  #:use-module (ice-9 match)
  #:use-module (ice-9 ftw)
  #:use-module (srfi srfi-1)
  #:use-module (sxml simple))

;; Returns a <ul> of changelog entries for the given
;; title (Added/Fixed/etc.) and given changelog info
;; for a single release
(define (get-list-for-changelog-group
          changelog-nfo title)
  (let*
    ((last-title ""))
    (fold
      (lambda (x accumulator)
        (if (string-contains x "###")
          (begin
            (set!
              last-title
              (string-replace-substring
                x "### " ""))
            accumulator)
          (if (string=? last-title title)
            (let*
              ((line
                 (string-replace-substring
                   x "- " "")))
              (if (> (string-length line) 0)
                (append accumulator `((li ,line)))
                accumulator))
            accumulator)))
      '()
      (string-split-substring changelog-nfo "\n"))))

;; Returns a list of the last 4 releases
(define (get-releases)
  (let*
    ((changelog
       (file-to-string
         (join-path
           '("@MESON_SOURCE_ROOT@"
             "CHANGELOG.md"))))
     (changelog-list
       (cdr (string-split-substring changelog "## [")))
     (releases-list `()))
    (for-each
      (lambda (x)
        (let*
          ((ver (car (string-split-substring x "]")))
           (str-from-date
             (car
               (cdr
                 (string-split-substring x " - "))))
           (date (car (string-split-substring str-from-date "\n")))
           (changelog-nfo
             (string-join
               (cdr (string-split-substring x "\n"))
               "\n")))
          (set!
            releases-list
            (append
              releases-list
              `((release
                  (@ (date ,date)
                     (version ,ver)
                     (type "development"))
                  (url
                    ,(string-append
                       "@RELEASE_TAG_BASE_URL@/v" ver))
                  (description
                    ,(fold
                      (lambda (x accumulator)
                        (if
                          (string-contains x "###")
                          (let*
                            ((title
                               (string-replace-substring
                                 x "### " "")))
                            (append
                              accumulator
                              `((p
                                  ,(string-append
                                     title ":"))
                                (ul
                                  ,@(get-list-for-changelog-group
                                      changelog-nfo
                                      title)))))
                          accumulator))
                      '()
                      (string-split-substring changelog-nfo "\n")))))))))
      changelog-list)
    (list-head releases-list 4)))

#!
Args:
1: output file
2: app ID
!#
(define (main . args)

  ;; verify number of args
  (unless (eq? (length args) 3)
    (display "Need 2 arguments")
    (newline)
    (exit -1))

  ;; get args
  (match args
    ((this-program output-file app-id)

     (with-output-to-file output-file
       (lambda ()

           ;; write XML
           (sxml->xml
            `(*TOP*
               (*PI*
                 xml
                 "version=\"1.0\" encoding=\"UTF-8\"")
               ;; TODO insert: Copyright 2022 Alexandros Theodotou
               (component
                 (@ (type "desktop-application"))
                 (id "org.zrythm.Zrythm")
                 ;; The tag 'metadata_license' means
                 ;; the licence of this file, not the
                 ;;whole product
                 (metadata_license "CC0-1.0")
                 (project_license  "AGPL-3.0-or-later")
                 (name "Zrythm")
                 (developer_name
                   "The Zrythm project")
                 (summary "Digital audio workstation")
                 (description
                   (p "Zrythm is a digital audio
workstation designed to
be featureful and easy to use.
It offers streamlined editing workflows with flexible
tools, limitless automation capabilities, powerful
mixing features, chord assistance and support for
various plugin and file formats."))
                 (categories
                   (category "AudioVideo")
                   (category "Audio"))
                 (url
                   (@ (type "homepage"))
                   "@HOMEPAGE_URL@")
                 (url
                   (@ (type "bugtracker"))
                   "@BUG_REPORT_URL@")
                 (url
                   (@ (type "faq"))
                   "@FAQ_URL@")
                 (url
                   (@ (type "help"))
                   "@CHATROOM_URL@")
                 (url
                   (@ (type "donation"))
                   "@DONATION_URL@")
                 (url
                   (@ (type "translate"))
                   "@TRANSLATE_URL@")
                 (url
                   (@ (type "contact"))
                   "@CONTACT_URL@")
                 (url
                   (@ (type "vcs-browser"))
                   "@MAIN_REPO_URL@")
                 (launchable
                   (@ (type "desktop-id"))
                   ,(string-append app-id
                                   ".desktop"))
                 (screenshots
                   (screenshot
                     (@ (type "default"))
                     (image
                       (@ (type "source"))
                       "@MAIN_SCREENSHOT_URL@")
                     (caption
                       "Main window with plugins")))
                 (update_contact "alex_at_zrythm.org")
                 (keywords
                   (keyword "Zrythm")
                   (keyword "DAW"))
                 (kudos
                   (kudo "HiDpiIcon")
                   (kudo "ModernToolkit"))
                 (project_group "Zrythm")
                 (translation
                   (@ (type "gettext"))
                   "zrythm")
                 (provides
                   (binary "zrythm_launch"))
                 (branding
                   (color
                     (@ (type "primary")
                        (scheme_preference "light"))
                     "#FF8B73")
                   (color
                     (@ (type "primary")
                        (scheme_preference "dark"))
                     "#B12408"))

                 (releases
                   ,@(get-releases))

                 (content_rating
                   (@ (type "oars-1.1")))))))))))

(apply main (program-arguments))
